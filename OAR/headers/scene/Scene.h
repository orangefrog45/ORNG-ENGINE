#pragma once
#include "scene/Skybox.h"
#include "terrain/Terrain.h"
#include "scene/GridMesh.h"
#include "rendering/MeshAsset.h"
#include "components/lights/BaseLight.h"
#include "components/lights/DirectionalLight.h"
#include "components/MeshComponent.h"
#include "components/lights/PointLightComponent.h"
#include "components/lights/SpotLightComponent.h"
#include "components/ScriptComponent.h"
#include "util/Log.h"
#include "scene/MeshInstanceGroup.h"
#include "scene/GlobalFog.h"
#include "components/ComponentManagers.h"

namespace ORNG {

	class SceneEntity;


	class Scene {
	public:
		friend class EditorLayer;
		friend class Renderer;
		friend class Component;
		friend class SceneRenderer;
		Scene();
		~Scene();

		SceneEntity& CreateEntity(const char* name = "Unnamed entity");

		//Runs any scripts in script components and updates transform and instance group data
		void Update();


		template<std::derived_from<Component> T, typename... Args>
		T* AddComponent(SceneEntity* p_entity, Args... args) {
			T* comp = nullptr;

			if constexpr (std::is_same<T, MeshComponent>::value) {
				comp = AddMeshComponent(p_entity, args...);
			}
			else if constexpr (std::is_same<T, PointLightComponent>::value) {
				comp = m_pointlight_component_manager.AddComponent(p_entity);
			}
			else if constexpr (std::is_same<T, SpotLightComponent>::value) {
				comp = m_spotlight_component_manager.AddComponent(p_entity);
			}
			else if constexpr (std::is_same<T, ScriptComponent>::value) {
				comp = AddScriptComponent(p_entity);
			}
			else if constexpr (std::is_same<T, CameraComponent>::value) {
				comp = AddCameraComponent(p_entity);
			}

			return comp;
		};

		template<std::derived_from<Component> T>
		T* GetComponent(unsigned long entity_id) {

			if constexpr (std::is_same<T, MeshComponent>::value) {
				return m_mesh_component_manager.GetComponent(entity_id);
			}
			else if constexpr (std::is_same<T, PointLightComponent>::value) {
				return m_pointlight_component_manager.GetComponent(entity_id);
			}
			else if constexpr (std::is_same<T, SpotLightComponent>::value) {
				return m_spotlight_component_manager.GetComponent(entity_id);
			}
			else if constexpr (std::is_same<T, ScriptComponent>::value) {
				auto it = std::find_if(m_script_components.begin(), m_script_components.end(), [&](const auto& p_comp) {return p_comp->GetEntityHandle() == entity_id; });
				return it == m_script_components.end() ? nullptr : *it;
			}
			else if constexpr (std::is_same<T, CameraComponent>::value) {
				auto it = std::find_if(m_camera_components.begin(), m_camera_components.end(), [&](const auto& p_comp) {return p_comp->GetEntityHandle() == entity_id; });
				return it == m_camera_components.end() ? nullptr : *it;
			}
			else if constexpr (std::is_same<T, TransformComponent>::value) {
				return m_transform_component_manager.GetComponent(entity_id);
			}
		}

		template<std::derived_from<Component> T>
		void DeleteComponent(SceneEntity* p_entity) {

			if constexpr (std::is_same<T, MeshComponent>::value) {
				m_mesh_component_manager.DeleteComponent(p_entity);
			}
			else if constexpr (std::is_same<T, PointLightComponent>::value) {
				m_pointlight_component_manager.DeleteComponent(p_entity);
			}
			else if constexpr (std::is_same<T, SpotLightComponent>::value) {
				m_spotlight_component_manager.DeleteComponent(p_entity);
			}
			else if constexpr (std::is_same<T, ScriptComponent>::value) {
				//auto it = std::find_if(m_script_components.begin(), m_script_components.end(), [&](const auto& p_comp) {return p_comp->GetEntityHandle() == p_entity->GetID(); });
				//delete* it;
				//m_script_components.erase(it);
			}

		}

		void MakeCameraActive(CameraComponent* p_cam);

		MeshComponent* AddMeshComponent(SceneEntity* p_entity, const std::string& filename);
		ScriptComponent* AddScriptComponent(SceneEntity* p_entity);
		CameraComponent* AddCameraComponent(SceneEntity* p_entity);


		// Creates material and returns ID, get actual material with GetMaterial(id)
		Material* CreateMaterial();
		MeshAsset* CreateMeshAsset(const std::string& filename);
		Texture2D* CreateTexture2DAsset(const std::string& filename, bool srgb);

		/* Removes asset from all components using it, then deletes asset */
		void DeleteMeshAsset(MeshAsset* data);

		inline Material* GetMaterial(unsigned int id) const {
			if (id < m_materials.size()) {
				return m_materials[id];
			}
			else {
				OAR_CORE_ERROR("Material with id '{0}' does not exist or is somehow out of range", id);
				return nullptr;
			}

		}

		SceneEntity* GetEntity(unsigned long id);

		[[nodiscard]] unsigned long CreateEntityID() {
			return m_last_entity_id++;
		}

		void LoadScene();
		void UnloadScene();

	private:
		void LoadMeshAssetIntoGPU(MeshAsset* asset);
		Texture2D* LoadMeshAssetTexture(const std::string& dir, aiTextureType type, const aiMaterial* material);

		BaseLight m_global_ambient_lighting = BaseLight(0);
		DirectionalLight m_directional_light;

		CameraComponent* mp_active_camera = nullptr;


		std::vector<SceneEntity*> m_entities;
		std::vector<ScriptComponent*> m_script_components;
		std::vector<CameraComponent*> m_camera_components;

		MeshComponentManager m_mesh_component_manager;
		PointlightComponentManager m_pointlight_component_manager;
		SpotlightComponentManager m_spotlight_component_manager;
		TransformComponentManager m_transform_component_manager;

		std::vector<Material*> m_materials; // materials referenced using id's, which is the materials position in this vector
		std::vector<MeshAsset*> m_mesh_assets;
		std::vector<Texture2D*> m_texture_2d_assets;

		Terrain m_terrain;
		Skybox m_skybox;
		GlobalFog m_global_fog;

		float m_exposure_level = 1.0f;

		std::vector<std::future<void>> m_futures;
		unsigned long m_last_entity_id = 1; // Last ID assigned to a newly created entity
	};

}