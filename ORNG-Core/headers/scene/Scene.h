#pragma once
#include "scene/Skybox.h"
#include "terrain/Terrain.h"
#include "scene/GridMesh.h"
#include "rendering/MeshAsset.h"
#include "scene/MeshInstanceGroup.h"
#include "components/ComponentSystems.h"
#include "scene/ScenePostProcessing.h"
#include "../extern/entt/EnttSingleInclude.h"

namespace ORNG {

	class SceneEntity;

	class Scene {
	public:
		friend class EditorLayer;
		friend class SceneRenderer;
		friend class SceneSerializer;
		friend class SceneEntity;
		Scene() = default;
		~Scene();

		void Update(float ts);

		// Specify uuid if deserializing
		SceneEntity& CreateEntity(const std::string& name, uint64_t uuid = 0);
		void DeleteEntity(SceneEntity* p_entity);
		SceneEntity* GetEntity(uint64_t uuid);
		SceneEntity* GetEntity(const std::string& name);
		SceneEntity* GetEntity(entt::entity handle);

		Skybox skybox;
		Terrain terrain;
		PostProcessing post_processing;

		void LoadScene(const std::string& filepath);
		void UnloadScene();

		UUID uuid;
	private:
		bool m_is_loaded = false;

		BaseLight m_global_ambient_lighting = BaseLight(0);
		DirectionalLight m_directional_light;

		std::vector<SceneEntity*> m_entities;

		MeshInstancingSystem m_mesh_component_manager{ &m_registry, uuid() };
		PointlightSystem m_pointlight_component_manager{ &m_registry , uuid() };
		SpotlightSystem m_spotlight_component_manager{ &m_registry, uuid() };
		PhysicsSystem m_physics_system{ &m_registry, uuid() };
		CameraSystem m_camera_system{ &m_registry, uuid() };
		TransformHierarchySystem m_transform_system{ &m_registry, uuid() };

		entt::registry m_registry;
		std::string m_name = "Untitled scene";

		std::vector<std::future<void>> m_futures;
	};

}