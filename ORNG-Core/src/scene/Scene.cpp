#include "pch/pch.h"

#include "util/TimeStep.h"
#include "scene/Scene.h"
#include "util/util.h"
#include "core/GLStateManager.h"
#include "scene/SceneEntity.h"
#include "../extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD.h"
#include "scene/SceneSerializer.h"
#include "core/CodedAssets.h"
#include "rendering/EnvMapLoader.h"
#include "core/AssetManager.h"


namespace ORNG {


	Scene::~Scene() {
		UnloadScene();
	}

	void Scene::Update(float ts) {

		m_physics_system.OnUpdate(ts);

		m_mesh_component_manager.OnUpdate();
		m_camera_system.OnUpdate();

		for (auto [entity, script] : m_registry.view<ScriptComponent>().each()) {
			script.OnUpdate(script.GetEntity(), this);
		}

		if (m_camera_system.GetActiveCamera())
			terrain.UpdateTerrainQuadtree(m_camera_system.GetActiveCamera()->GetEntity()->GetComponent<TransformComponent>()->GetPosition());
	}




	void Scene::DeleteEntity(SceneEntity* p_entity) {
		auto it = std::ranges::find(m_entities, p_entity);

		auto& current_child_entity = p_entity->GetComponent<RelationshipComponent>()->first;
		while (current_child_entity != entt::null) {
			auto* p_child_ent = GetEntity(current_child_entity);
			entt::entity next = p_child_ent->GetComponent<RelationshipComponent>()->next;
			DeleteEntity(p_child_ent);
			current_child_entity = next;
		}


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

		if (!SceneSerializer::DeserializeScene(*this, filepath)) {
			EnvMapLoader::LoadEnvironmentMap("", skybox, 1);
		}
		m_is_loaded = true;
		ORNG_CORE_INFO("Scene loaded in {0}ms", time.GetTimeInterval());
	}




	void Scene::UnloadScene() {
		ORNG_CORE_INFO("Unloading scene...");
		m_is_loaded = false;


		while (!m_entities.empty()) {
			// Safe deletion method
			DeleteEntity(m_entities[0]);
		}

		m_registry.clear();
		m_transform_system.OnUnload();
		m_physics_system.OnUnload();
		m_mesh_component_manager.OnUnload();
		m_camera_system.OnUnload();

		m_entities.clear();
		ORNG_CORE_INFO("Scene unloaded");
	}


	SceneEntity& Scene::CreateEntity(const std::string& name, uint64_t uuid) {
		// Choose to create with uuid or not
		auto reg_ent = m_registry.create();
		SceneEntity* ent = uuid == 0 ? new SceneEntity(this, reg_ent) : new SceneEntity(uuid, reg_ent, this);
		ent->name = name;
		m_entities.push_back(ent);

		return *ent;

	}



}