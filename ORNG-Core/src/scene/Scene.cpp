
#include "pch/pch.h"
#include "util/TimeStep.h"
#include "scene/Scene.h"
#include "util/util.h"
#include "scene/SceneEntity.h"
#include "../extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD.h"
#include "assets/AssetManager.h"
#include "scene/SceneSerializer.h"
#include "util/Timers.h"
#include "core/FrameTiming.h"


namespace ORNG {
	Scene::~Scene() {
		if (m_is_loaded)
			UnloadScene();
	}

	void Scene::Update(float ts) {

		m_camera_system.OnUpdate();
		m_audio_system.OnUpdate();

		m_particle_system.OnUpdate();
		physics_system.OnUpdate(ts);

		SetScriptState();
		ORNG_PROFILE_FUNC();
		for (auto [entity, script] : m_registry.view<ScriptComponent>().each()) {
			try {
				script.p_instance->OnUpdate();
			}
			catch (std::exception& e) {
				ORNG_CORE_ERROR("Script execution error with script '{0}' : '{1}'", script.GetSymbols()->script_path, e.what());
			}
		}

		m_mesh_component_manager.OnUpdate();
		//if (m_camera_system.GetActiveCamera())
			//terrain.UpdateTerrainQuadtree(m_camera_system.GetActiveCamera()->GetEntity()->GetComponent<TransformComponent>()->GetPosition());


		for (auto* p_entity : m_entity_deletion_queue) {
			DeleteEntity(p_entity);
		}


		m_entity_deletion_queue.clear();
	}



	void Scene::DeleteEntity(SceneEntity* p_entity) {
		ASSERT(m_registry.valid(p_entity->GetEnttHandle()));

		auto current_child_entity = p_entity->GetComponent<RelationshipComponent>()->first;
		while (current_child_entity != entt::null) {
			auto* p_child_ent = GetEntity(current_child_entity);
			entt::entity next = p_child_ent->GetComponent<RelationshipComponent>()->next;
			DeleteEntity(p_child_ent);
			current_child_entity = next;
		}


		auto it = std::ranges::find(m_entities, p_entity);
		ASSERT(it != m_entities.end());
		delete p_entity;
		m_entities.erase(it);
	}


	SceneEntity* Scene::GetEntity(uint64_t uuid) {
		auto it = std::find_if(m_entities.begin(), m_entities.end(), [&](const auto* p_entity) {return p_entity->GetUUID() == uuid; });
		return it == m_entities.end() ? nullptr : *it;
	}

	SceneEntity* Scene::GetEntity(const std::string& name) {
		auto it = std::find_if(m_entities.begin(), m_entities.end(), [&](const auto* p_entity) {return p_entity->name == name; });
		return it == m_entities.end() ? nullptr : *it;
	}
	SceneEntity* Scene::GetEntity(entt::entity handle) {
		auto it = std::find_if(m_entities.begin(), m_entities.end(), [&](const auto* p_entity) {return p_entity->m_entt_handle == handle; });
		return it == m_entities.end() ? nullptr : *it;
	}


	SceneEntity& Scene::InstantiatePrefabCallScript(const std::string& serialized_data) {
		auto& ent = InstantiatePrefab(serialized_data);
		if (auto* p_script = ent.GetComponent<ScriptComponent>())
			p_script->p_instance->OnCreate();

		return ent;
	}

	SceneEntity& Scene::InstantiatePrefab(const std::string& serialized_data) {
		auto vec = SceneSerializer::DeserializePrefabFromString(*this, serialized_data);
		
		// Prefab entities are serialized so that the root/top-parent is first
		return *vec[0];
	}



	SceneEntity& Scene::DuplicateEntity(SceneEntity& original) {
		SceneEntity& new_entity = CreateEntity(original.name + " - Duplicate");
		std::string str = SceneSerializer::SerializeEntityIntoString(original);
		SceneSerializer::DeserializeEntityFromString(*this, str, new_entity);

		auto* p_relation_comp = original.GetComponent<RelationshipComponent>();
		SceneEntity* p_current_child = GetEntity(p_relation_comp->first);
		while (p_current_child) {
			DuplicateEntity(*p_current_child).SetParent(new_entity);
			p_current_child = GetEntity(p_current_child->GetComponent<RelationshipComponent>()->next);
		}
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


	SceneEntity& Scene::DuplicateEntityCallScript(SceneEntity& original) {
		auto& ent = DuplicateEntity(original);
		if (auto* p_script = ent.GetComponent<ScriptComponent>())
			p_script->p_instance->OnCreate();

		return ent;
	}

	void Scene::LoadScene(const std::string& filepath) {
		TimeStep time = TimeStep(TimeStep::TimeUnits::MILLISECONDS);

		FastNoiseSIMD* noise = FastNoiseSIMD::NewFastNoiseSIMD();
		noise->SetFrequency(0.05f);
		noise->SetCellularReturnType(FastNoiseSIMD::Distance);
		noise->SetNoiseType(FastNoiseSIMD::PerlinFractal);


		post_processing.global_fog.SetNoise(noise);
		terrain.Init(AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID));
		physics_system.OnLoad();
		m_mesh_component_manager.OnLoad();
		m_camera_system.OnLoad();
		m_transform_system.OnLoad();
		m_audio_system.OnLoad();
		m_particle_system.OnLoad();

		// Allocate storage for components on this side of the boundary
		// If not done allocation will be done in dll's (scripts), if the dlls have to reload or disconnect the memory is invalidated, so allocations must be done here
		auto& ent = CreateEntity("allocator");
		ent.AddComponent<MeshComponent>();
		ent.AddComponent<PointLightComponent>();
		ent.AddComponent<SpotLightComponent>();
		ent.AddComponent<ScriptComponent>();
		ent.AddComponent<DataComponent>();
		ent.AddComponent<PhysicsComponent>();
		ent.AddComponent<CharacterControllerComponent>();
		ent.AddComponent<CameraComponent>();
		ent.AddComponent<AudioComponent>();
		ent.AddComponent<VehicleComponent>();
		ent.AddComponent<ParticleEmitterComponent>();
		ent.AddComponent<ParticleBufferComponent>();
		DeleteEntity(&ent);

		m_is_loaded = true;
		ORNG_CORE_INFO("Scene loaded in {0}ms", time.GetTimeInterval());
	}


	void Scene::OnStart() {
		SetScriptState();
		for (auto [entity, script] : m_registry.view<ScriptComponent>().each()) {
			try {
				script.p_instance->OnCreate();
			}
			catch (std::exception e) {
				ORNG_CORE_ERROR("Script execution error for entity '{0}' : '{1}'", GetEntity(entity)->name, e.what());
			}
		}
		ORNG_CORE_TRACE("FRAMETIMING : {0}", FrameTiming::GetTotalElapsedTime());
	}

	void Scene::SetScriptState() {
		for (auto* p_script_asset : AssetManager::GetView<ScriptAsset>()) {
			p_script_asset->symbols.SceneGetEntityEnttHandleSetter([this](entt::entity entt_handle) -> SceneEntity& {
				auto* p_ent = GetEntity(entt_handle);

				if (!p_ent)
					throw std::runtime_error(std::format("Scene::GetEntity failed, entity with entt handle {} not found", (uint32_t)entt_handle)); // Caught in Scene::Update

				return *p_ent;
				});

			p_script_asset->symbols.SceneEntityCreationSetter([this](const std::string& str) -> SceneEntity& {
				return CreateEntity(str);
				});

			p_script_asset->symbols.SceneEntityDeletionSetter([this](SceneEntity* p_entity) {
				if (!VectorContains(m_entity_deletion_queue, p_entity))
					m_entity_deletion_queue.push_back(p_entity);
				});

			p_script_asset->symbols.SceneEntityDuplicationSetter([this](SceneEntity& ent) -> SceneEntity& {
				return DuplicateEntityCallScript(ent);
				});

			p_script_asset->symbols.ScenePrefabInstantSetter([this](const std::string& str) -> SceneEntity& {
				return InstantiatePrefabCallScript(str);
				});

			p_script_asset->symbols.SceneRaycastSetter([this](glm::vec3 origin, glm::vec3 unit_dir, float max_distance) -> RaycastResults {
				return physics_system.Raycast(origin, unit_dir, max_distance);
				});

			p_script_asset->symbols.SceneGetEntitySetter([this](uint64_t id) -> SceneEntity& {
				auto* p_ent = GetEntity(id);

				if (!p_ent)
					throw std::runtime_error(std::format("Scene::GetEntity failed, entity with uuid {} not found", id)); // Caught in Scene::Update

				return *p_ent;
				});

			p_script_asset->symbols.SceneOverlapQuerySetter([this](glm::vec3 pos, float range) -> OverlapQueryResults {
				return physics_system.OverlapQuery(pos, range);
				});
		}
	}


	void Scene::UnloadScene() {
		if (!m_is_loaded) {
			return;
		}
		ORNG_CORE_INFO("Unloading scene...");


		while (!m_entities.empty()) {
			// Safe deletion method
			DeleteEntity(m_entities[0]);
		}

		m_registry.clear();
		m_transform_system.OnUnload();
		physics_system.OnUnload();
		m_mesh_component_manager.OnUnload();
		m_camera_system.OnUnload();
		m_audio_system.OnUnload();
		m_particle_system.OnUnload();

		m_entities.clear();
		ORNG_CORE_INFO("Scene unloaded");
		m_is_loaded = false;
	}



	SceneEntity& Scene::CreateEntity(const std::string& name, uint64_t uuid) {
		auto reg_ent = m_registry.create();
		SceneEntity* ent = uuid == 0 ? new SceneEntity(this, reg_ent, &m_registry, this->uuid()) : new SceneEntity(uuid, reg_ent, this, &m_registry, this->uuid());
		ent->name = name;
		m_entities.push_back(ent);

		return *ent;
	}
}