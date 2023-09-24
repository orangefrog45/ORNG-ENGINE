#include "pch/pch.h"
#include "util/TimeStep.h"
#include "scene/Scene.h"
#include "util/util.h"
#include "core/GLStateManager.h"
#include "scene/SceneEntity.h"
#include "../extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD.h"
#include "assets/AssetManager.h"
#include "scene/SceneSerializer.h"
#include "util/Timers.h"


namespace ORNG {


	Scene::~Scene() {
		if (m_is_loaded)
			UnloadScene();
	}

	void Scene::Update(float ts) {

		ORNG_PROFILE_FUNC();

		m_camera_system.OnUpdate();
		m_audio_system.OnUpdate();


		// Script state updates need to be set far less frequently, use events for this soon
		SetScriptState();
		for (auto [entity, script] : m_registry.view<ScriptComponent>().each()) {
			try {
				script.p_symbols->OnUpdate(script.GetEntity());
			}
			catch (std::exception e) {
				ORNG_CORE_ERROR("Script execution error with script '{0}' : '{1}'", script.p_symbols->script_path, e.what());
			}
		}

		m_physics_system.OnUpdate(ts);
		m_mesh_component_manager.OnUpdate();

		if (m_camera_system.GetActiveCamera())
			terrain.UpdateTerrainQuadtree(m_camera_system.GetActiveCamera()->GetEntity()->GetComponent<TransformComponent>()->GetPosition());


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
		ORNG_CORE_TRACE("'{0}' DELETING", (uint32_t)p_entity->GetEnttHandle());
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
			p_script->p_symbols->OnCreate(&ent);

		return ent;
	}

	SceneEntity& Scene::InstantiatePrefab(const std::string& serialized_data) {
		auto& ent = CreateEntity("Prefab instantiation");
		SceneSerializer::DeserializeEntityFromString(*this, serialized_data, ent);

		return ent;
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


	SceneEntity& Scene::DuplicateEntityCallScript(SceneEntity& original) {
		auto& ent = DuplicateEntity(original);
		if (auto* p_script = ent.GetComponent<ScriptComponent>())
			p_script->p_symbols->OnCreate(&ent);

		return ent;
	}

	void Scene::LoadScene(const std::string& filepath) {
		TimeStep time = TimeStep(TimeStep::TimeUnits::MILLISECONDS);

		FastNoiseSIMD* noise = FastNoiseSIMD::NewFastNoiseSIMD();
		noise->SetFrequency(0.05f);
		noise->SetCellularReturnType(FastNoiseSIMD::Distance);
		noise->SetNoiseType(FastNoiseSIMD::PerlinFractal);


		post_processing.global_fog.SetNoise(noise);
		terrain.Init(AssetManager::GetEmptyMaterial());
		m_physics_system.OnLoad();
		m_mesh_component_manager.OnLoad();
		m_camera_system.OnLoad();
		m_transform_system.OnLoad();
		m_audio_system.OnLoad();

		m_is_loaded = true;
		ORNG_CORE_INFO("Scene loaded in {0}ms", time.GetTimeInterval());
	}


	void Scene::OnStart() {
		SetScriptState();
		for (auto [entity, script] : m_registry.view<ScriptComponent>().each()) {
			try {
				script.p_symbols->OnCreate(GetEntity(entity));
			}
			catch (std::exception e) {
				ORNG_CORE_ERROR("Script execution error for entity '{0}' : '{1}'", GetEntity(entity)->name, e.what());
			}
		}
	}

	void Scene::SetScriptState() {
		for (auto* p_script_asset : AssetManager::GetView<ScriptAsset>()) {
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
				return m_physics_system.Raycast(origin, unit_dir, max_distance);
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
		m_physics_system.OnUnload();
		m_mesh_component_manager.OnUnload();
		m_camera_system.OnUnload();
		m_audio_system.OnUnload();

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
