#pragma once
#include "scene/Skybox.h"
#include "terrain/Terrain.h"
#include "scene/GridMesh.h"
#include "components/ComponentSystems.h"
#include "scene/ScenePostProcessing.h"
#include "../extern/entt/EnttSingleInclude.h"

namespace ScriptInterface {
	class S_Scene;
}

namespace ORNG {

	class SceneEntity;


	class Scene {
	public:
		friend class EditorLayer;
		friend class SceneRenderer;
		friend class SceneSerializer;
		friend class SceneEntity;
		friend class AssetManagerWindow;
		Scene() = default;
		~Scene();

		void Update(float ts);

		// Specify uuid if deserializing
		SceneEntity& CreateEntity(const std::string& name, uint64_t uuid = 0);
		void DeleteEntity(SceneEntity* p_entity);
		SceneEntity* GetEntity(uint64_t uuid);
		SceneEntity* GetEntity(const std::string& name);
		SceneEntity* GetEntity(entt::entity handle);


		SceneEntity& DuplicateEntity(SceneEntity& original);



		inline RaycastResults Raycast(glm::vec3 origin, glm::vec3 unit_dir, float max_distance) {
			return m_physics_system.Raycast(origin, unit_dir, max_distance);
		}

		Skybox skybox;
		Terrain terrain;
		PostProcessing post_processing;
		DirectionalLight directional_light;

		void ClearAllEntities() {
			unsigned int max_iters = 10'000'000;
			unsigned int i = 0;
			while (!m_entities.empty()) {
				i++;

				if (i > max_iters) {
					ORNG_CORE_CRITICAL("Scene::ClearAllEntities exceeded max iteration count of 10,000,000, exiting");
					break;
				}

				DeleteEntity(m_entities[0]);
			}
		}

		void LoadScene(const std::string& filepath);
		void UnloadScene();

		UUID uuid;
	private:
		bool m_is_loaded = false;


		std::vector<SceneEntity*> m_entities;
		std::vector<SceneEntity*> m_entity_deletion_queue;


		MeshInstancingSystem m_mesh_component_manager{ &m_registry, uuid() };
		PhysicsSystem m_physics_system{ &m_registry, uuid(), this };
		CameraSystem m_camera_system{ &m_registry, uuid() };
		TransformHierarchySystem m_transform_system{ &m_registry, uuid() };
		AudioSystem m_audio_system{ &m_registry, uuid(), &m_camera_system.m_active_cam_entity_handle };

		entt::registry m_registry;

		std::string m_name = "Untitled scene";
	};

}