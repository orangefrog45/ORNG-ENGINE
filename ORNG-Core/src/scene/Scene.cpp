
#include "pch/pch.h"
#include "util/TimeStep.h"
#include "scene/Scene.h"
#include "util/util.h"
#include "scene/SceneEntity.h"
#include "../extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD.h"
#include "components/ComponentSystems.h"
#include "assets/AssetManager.h"
#include "scene/SceneSerializer.h"
#include "util/Timers.h"
#include "core/FrameTiming.h"
#include "Tracy.hpp"
#include "core/Window.h"

// For SI funcs
#include "rendering/Renderer.h"
#include "rendering/SceneRenderer.h" 

namespace ORNG {
	Scene::~Scene() {
		if (m_is_loaded)
			UnloadScene();
	}

	void Scene::AddDefaultSystems() {
		AddSystem(new CameraSystem{ this });
		AddSystem(new AudioSystem{ this });
		AddSystem(new ParticleSystem{ this });
		AddSystem(new PhysicsSystem{ this });
		AddSystem(new MeshInstancingSystem{ this });
		AddSystem(new TransformHierarchySystem{ this });
		AddSystem(new ScriptSystem{ this });
	}

	void Scene::Update(float ts) {
		m_time_elapsed += ts;

		for (auto [id, p_system] : systems) {
			p_system->OnUpdate();
		}
		
		for (auto [entity, script] : m_registry.view<ScriptComponent>().each()) {
			script.p_instance->OnUpdate(ts * 0.001f); // convert to seconds
		}

		//if (m_camera_system.GetActiveCamera())
			//terrain.UpdateTerrainQuadtree(m_camera_system.GetActiveCamera()->GetEntity()->GetComponent<TransformComponent>()->GetPosition());

		for (auto* p_entity : m_entity_deletion_queue) {
			DeleteEntity(p_entity);
		}

		m_entity_deletion_queue.clear();
	}

	void Scene::OnImGuiRender() {
		for (auto [entity, script] : m_registry.view<ScriptComponent>().each()) {
			script.p_instance->OnImGuiRender();
		}
	}


	void Scene::DeleteEntityAtEndOfFrame(SceneEntity* p_entity) {
		if (!VectorContains(m_entity_deletion_queue, p_entity))
			m_entity_deletion_queue.push_back(p_entity);
	}

	void Scene::DeleteEntity(SceneEntity* p_entity) {
		ASSERT(m_registry.valid(p_entity->GetEnttHandle()));

		if (auto* p_script = p_entity->GetComponent<ScriptComponent>(); p_script && p_script->p_instance) {
			p_script->p_instance->OnDestroy();
		}
		p_entity->RemoveParent();

		auto current_child_entity = p_entity->GetComponent<RelationshipComponent>()->first;
		while (current_child_entity != entt::null) {
			auto* p_child_ent = GetEntity(current_child_entity);

			entt::entity next = p_child_ent->GetComponent<RelationshipComponent>()->next;
			DeleteEntity(p_child_ent);
			current_child_entity = next;
		}

		if (m_root_entities.contains(p_entity->GetEnttHandle()))
			m_root_entities.erase(p_entity->GetEnttHandle());
		
		auto it = std::ranges::find(m_entities, p_entity);
		ASSERT(it != m_entities.end());

		m_entity_uuid_lookup.erase(p_entity->GetUUID());
		delete p_entity;
		m_entities.erase(it);
	}


	SceneEntity* Scene::GetEntity(uint64_t uuid) {
		if (!m_entity_uuid_lookup.contains(uuid)) return nullptr;

		return m_entity_uuid_lookup[uuid];
	}

	SceneEntity* Scene::GetEntity(const std::string& name) {
		auto it = std::find_if(m_entities.begin(), m_entities.end(), [&](const auto* p_entity) {return p_entity->name == name; });
		return it == m_entities.end() ? nullptr : *it;
	}

	SceneEntity* Scene::GetEntity(entt::entity handle) {
		auto* p_transform = m_registry.try_get<TransformComponent>(handle);

		if (!p_transform)
			return nullptr;
		else
			return p_transform->GetEntity();
	}

	SceneEntity& Scene::InstantiatePrefab(const Prefab& prefab, bool call_on_create) {
		auto vec = SceneSerializer::DeserializePrefab(*this, prefab);
		
		if (call_on_create) {
			for (auto* p_ent : vec) {
				if (auto* p_script = p_ent->GetComponent<ScriptComponent>(); p_script && p_script->p_instance) p_script->p_instance->OnCreate();
			}
		}

		// Prefab entities are serialized so that the root/top-parent is first
		return *vec[0];
	}

	SceneEntity* Scene::InstantiatePrefab(uint64_t prefab_uuid, bool call_on_create) {
		Prefab* p_prefab = AssetManager::GetAsset<Prefab>(prefab_uuid);
		return p_prefab ? &InstantiatePrefab(*p_prefab, call_on_create) : nullptr;
	}

	std::vector<SceneEntity*> Scene::DuplicateEntityGroup(const std::vector<SceneEntity*> group) {
		// Any UUID's of entities that are in "group" will be mapped to the new UUIDs of the duplicate entities, 
		// this means things like joint connections will connect to the new duplicates if found instead of the old entities
		std::unordered_map<uint64_t, uint64_t> uuid_lookup;

		std::vector<SceneEntity*> duplicates;
	
		for (int i = 0; i < group.size(); i++) {
			auto& ent = DuplicateEntityAsPartOfGroup(*group[i], uuid_lookup);
			ent.SetParent(*GetEntity(group[i]->GetComponent<RelationshipComponent>()->parent));
			duplicates.push_back(&ent);
		}

		// Re-map connections to newly duplicated entities
		SceneSerializer::RemapEntityReferences(uuid_lookup, duplicates);

		for (auto* p_dup : duplicates) {
			SceneSerializer::ResolveEntityRefs(*this, *p_dup);
		}

		return duplicates;
	}

	SceneEntity& Scene::DuplicateEntityAsPartOfGroup(SceneEntity& original, std::unordered_map<uint64_t, uint64_t>& uuid_map) {
		SceneEntity& new_entity = CreateEntity(original.name + " - Duplicate");
		uuid_map[original.GetUUID()] = new_entity.GetUUID();
		std::string str = SceneSerializer::SerializeEntityIntoString(original);
		SceneSerializer::DeserializeEntityFromString(*this, str, new_entity, true);

		original.ForEachLevelOneChild(
			[&](entt::entity e) {
				auto& child = DuplicateEntityAsPartOfGroup(*GetEntity(e), uuid_map);
				child.SetParent(new_entity);
			}
		);

		return new_entity;
	}

	SceneEntity& Scene::DuplicateEntity(SceneEntity& original) {
		SceneEntity& new_entity = CreateEntity(original.name + " - Duplicate");
		std::string str = SceneSerializer::SerializeEntityIntoString(original);
		SceneSerializer::DeserializeEntityFromString(*this, str, new_entity);

		std::unordered_map<uint64_t, uint64_t> uuid_map;
		uuid_map[original.GetUUID()] = new_entity.GetUUID();

		original.ForEachLevelOneChild(
			[&](entt::entity e) {
				auto& child = DuplicateEntityAsPartOfGroup(*GetEntity(e), uuid_map);
				child.SetParent(new_entity);
			}
		);

		new_entity.ForEachChildRecursive(
			[&](entt::entity e) {
				auto* p_child = GetEntity(e);
				SceneSerializer::RemapEntityReferences(uuid_map, *p_child);
				SceneSerializer::ResolveEntityRefs(*this, *p_child);
			}
			
		);

		SceneSerializer::RemapEntityReferences(uuid_map, new_entity);
		SceneSerializer::ResolveEntityRefs(*this, new_entity);

		return new_entity;
	}

	void Scene::SortEntitiesNumParents(Scene* p_scene, std::vector<uint64_t>& entity_uuids, bool descending) {
		std::ranges::sort(entity_uuids, [&](const uint64_t& id_left, const uint64_t& id_right) {
			unsigned num_children_left = 0;
			unsigned num_children_right = 0;

			p_scene->GetEntity(id_left)->ForEachChildRecursive([&](entt::entity handle) {num_children_left++; });
			p_scene->GetEntity(id_right)->ForEachChildRecursive([&](entt::entity handle) {num_children_right++; });

			return descending ? num_children_left < num_children_right : num_children_left > num_children_right;

			});
	}

	void Scene::SortEntitiesNumParents(std::vector<SceneEntity*>& entities, bool descending) {
		std::ranges::sort(entities, [&](SceneEntity*& p_ent_left, SceneEntity*& p_ent_right) {
			unsigned num_children_left = 0;
			unsigned num_children_right = 0;

			p_ent_left->ForEachChildRecursive([&](entt::entity handle) {num_children_left++; });
			p_ent_right->ForEachChildRecursive([&](entt::entity handle) {num_children_right++; });

			return descending ? num_children_left < num_children_right : num_children_left > num_children_right;
		});
	}

	SceneEntity* Scene::TryFindRootEntityByName(const std::string& name) {
		for (auto handle : m_root_entities) {
			auto* p_ent = GetEntity(handle);
			if (p_ent->name == name)
				return p_ent;
		}

		return nullptr;
	}


	SceneEntity* Scene::TryFindEntity(const EntityNodeRef& ref) {
		SceneEntity* p_current_ent = nullptr;
		unsigned start_index = 0;

		auto& instructions = ref.GetInstructions();

		if (instructions.empty())
			return nullptr;

		if (instructions[0] == "::") { // Path is absolute
			p_current_ent = TryFindRootEntityByName(instructions[1]);
			start_index = 2;
		}
		else { // Path is relative to src
			p_current_ent = ref.GetSrc();
		}

		for (int i = start_index; i < instructions.size(); i++) {
			if (p_current_ent == nullptr)
				return p_current_ent;

			if (instructions[i] == "..") {
				p_current_ent = GetEntity(p_current_ent->GetParent());
			}
			else {
				p_current_ent = p_current_ent->GetChild(instructions[i]);
			}
		}

		return p_current_ent;
	}


	EntityNodeRef Scene::GenEntityNodeRef(SceneEntity* p_src, SceneEntity* p_target) {
		std::vector<std::string> instructions = { p_target->name };

		std::set<SceneEntity*> src_parents;
		std::vector<SceneEntity*> ordered_src_parents;

		SceneEntity* p_highest_shared_parent = nullptr;

		{
			SceneEntity* p_current_parent = GetEntity(p_src->GetParent());

			// Gather all parents of src entity
			while (p_current_parent) {
				src_parents.insert(p_current_parent);
				ordered_src_parents.push_back(p_current_parent);
				p_current_parent = GetEntity(p_current_parent->GetParent());
			}

			p_current_parent = GetEntity(p_target->GetParent());

			// Traverse all parents of target entity to check if p_target and p_src are related (parent/child)
			while (p_current_parent) {
				if (src_parents.contains(p_current_parent)) {
					p_highest_shared_parent = p_current_parent;
					break;
				}
				else if (p_current_parent == p_src) {
					// Instructions are already relative to p_src as p_target is a child of p_src
					break;
				}
				else {
					instructions.push_back(p_current_parent->name);
				}

				p_current_parent = GetEntity(p_current_parent->GetParent());
			}
		}


		// This idx is how many layers up needed to move until common parent is found
		if (p_highest_shared_parent) {
			unsigned idx = VectorFindIndex(ordered_src_parents, p_highest_shared_parent) + 1;

			// Get path relative to the first common parent
			for (int i = 0; i < idx; i++) {
				instructions.push_back("..");
			}

		}
		else {
			// Entities are unrelated so use an absolute path
			instructions.push_back("::");
		}
		

		// Instruction set is generated in reverse, so reverse here to correct it
		std::ranges::reverse(instructions);

		EntityNodeRef ref{ p_src, instructions };

		return ref;
	}


	SceneEntity& Scene::DuplicateEntityCallScript(SceneEntity& original) {
		auto& ent = DuplicateEntity(original);
		if (auto* p_script = ent.GetComponent<ScriptComponent>())
			p_script->p_instance->OnCreate();

		return ent;
	}

	void Scene::LoadScene() {
		TimeStep time = TimeStep(TimeStep::TimeUnits::MILLISECONDS);

		m_uuid_change_listener.OnEvent = [this](const UUIDChangeEvent& _event) {
			auto* p_ent = m_entity_uuid_lookup[_event.old_uuid];
			if (!p_ent) // Entity belongs to a different scene
				return;

			m_entity_uuid_lookup.erase(_event.old_uuid);
			m_entity_uuid_lookup[_event.new_uuid] = p_ent;
			};

		Events::EventManager::RegisterListener(m_uuid_change_listener);

		m_hierarchy_modification_listener.scene_id = uuid();
		m_hierarchy_modification_listener.OnEvent = [&](const Events::ECS_Event<RelationshipComponent>& _event) {
			entt::entity entity = _event.p_component->GetEnttHandle();
			entt::entity parent = _event.p_component->GetEntity()->GetParent();

			if (parent != entt::null && m_root_entities.contains(entity))
				m_root_entities.erase(entity);
			else if (parent == entt::null && !m_root_entities.contains(entity))
				m_root_entities.insert(entity);
			};

		Events::EventManager::RegisterListener(m_hierarchy_modification_listener);

		// Allocate storage for components from the main application
		// If not done allocation will be done in dll's (scripts), if the dlls have to reload or disconnect the memory is invalidated, so allocations must be done here
		RegisterComponent<MeshComponent>();
		RegisterComponent<PointLightComponent>();
		RegisterComponent<SpotLightComponent>();
		RegisterComponent<ScriptComponent>();
		RegisterComponent<DataComponent>();
		RegisterComponent<PhysicsComponent>();
		RegisterComponent<CharacterControllerComponent>();
		RegisterComponent<CameraComponent>();
		RegisterComponent<AudioComponent>();
		RegisterComponent<VehicleComponent>();
		RegisterComponent<ParticleEmitterComponent>();
		RegisterComponent<ParticleBufferComponent>();

		for (auto [id, p_sys] : systems) {
			p_sys->OnLoad();
		}

		m_is_loaded = true;
		ORNG_CORE_INFO("Scene loaded in {0}ms", time.GetTimeInterval());
	}


	CameraComponent* Scene::GetActiveCamera() {
		return GetSystem<CameraSystem>().GetActiveCamera();
	}

	void Scene::Start() {
		for (auto [entity, script] : m_registry.view<ScriptComponent>().each()) {
			script.p_instance->OnCreate();
		}
	}

	void Scene::UnloadScene() {
		ORNG_CORE_INFO("Unloading scene...");
		m_time_elapsed = 0.0;

		while (!m_entities.empty()) {
			// Safe deletion method
			DeleteEntity(m_entities[0]);
		}

		for (auto [id, p_sys] : systems) {
			p_sys->OnUnload();
			delete p_sys;
		}

		Events::EventManager::DeregisterListener(m_hierarchy_modification_listener.GetRegisterID());

		m_entities.clear();
		m_root_entities.clear();
		m_registry.clear();

		ORNG_CORE_INFO("Scene unloaded");
		m_is_loaded = false;
	}



	SceneEntity& Scene::CreateEntity(const std::string& name, uint64_t uuid) {
		auto reg_ent = m_registry.create();
		SceneEntity* ent = uuid == 0 ? new SceneEntity(this, reg_ent, &m_registry, this->uuid()) : new SceneEntity(uuid, reg_ent, this, &m_registry, this->uuid());
		uuid = ent->GetUUID();
		ent->name = name;
		m_entities.push_back(ent);
		DEBUG_ASSERT(!m_entity_uuid_lookup.contains(uuid))
		m_entity_uuid_lookup[uuid] = ent;

		// Entity initially has no parent so insert it here
		m_root_entities.insert(ent->GetEnttHandle());

		return *ent;
	}

}