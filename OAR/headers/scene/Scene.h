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
#include "rendering/MeshInstanceGroup.h"
#include "scene/GlobalFog.h"

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
		T* AddComponent(unsigned long entity_id, Args... args) {
			T* comp = nullptr;

			if constexpr (std::is_same<T, MeshComponent>::value) {
				comp = AddMeshComponent(entity_id, args...);
			}
			else if constexpr (std::is_same<T, PointLightComponent>::value) {
				comp = AddPointLightComponent(entity_id);
			}
			else if constexpr (std::is_same<T, SpotLightComponent>::value) {
				comp = AddSpotLightComponent(entity_id);
			}
			else if constexpr (std::is_same<T, ScriptComponent>::value) {
				comp = AddScriptComponent(entity_id);
			}
			else if constexpr (std::is_same<T, CameraComponent>::value) {
				comp = AddCameraComponent(entity_id);
			}

			return comp;
		};

		template<std::derived_from<Component> T>
		T* GetComponent(unsigned long entity_id) {

			if constexpr (std::is_same<T, MeshComponent>::value) {
				auto it = std::find_if(m_mesh_components.begin(), m_mesh_components.end(), [&](const auto& p_comp) {return p_comp->GetEntityHandle() == entity_id; });
				return it == m_mesh_components.end() ? nullptr : *it;
			}
			else if constexpr (std::is_same<T, PointLightComponent>::value) {
				auto it = std::find_if(m_point_lights.begin(), m_point_lights.end(), [&](const auto& p_comp) {return p_comp->GetEntityHandle() == entity_id; });
				return it == m_point_lights.end() ? nullptr : *it;
			}
			else if constexpr (std::is_same<T, SpotLightComponent>::value) {
				auto it = std::find_if(m_spot_lights.begin(), m_spot_lights.end(), [&](const auto& p_comp) {return p_comp->GetEntityHandle() == entity_id; });
				return it == m_spot_lights.end() ? nullptr : *it;
			}
			else if constexpr (std::is_same<T, ScriptComponent>::value) {
				auto it = std::find_if(m_script_components.begin(), m_script_components.end(), [&](const auto& p_comp) {return p_comp->GetEntityHandle() == entity_id; });
				return it == m_script_components.end() ? nullptr : *it;
			}
			else if constexpr (std::is_same<T, CameraComponent>::value) {
				auto it = std::find_if(m_camera_components.begin(), m_camera_components.end(), [&](const auto& p_comp) {return p_comp->GetEntityHandle() == entity_id; });
				return it == m_camera_components.end() ? nullptr : *it;
			}
		}

		template<std::derived_from<Component> T>
		void DeleteComponent(unsigned long entity_id) {

			if constexpr (std::is_same<T, MeshComponent>::value) {

				MeshInstanceGroup* group = m_mesh_components[entity_id]->mp_instance_group;
				MeshComponent* mesh = m_mesh_components[entity_id];

				group->DeleteMeshPtr(mesh);
				delete mesh;
				mesh = nullptr;

				OAR_CORE_TRACE("Mesh component deleted from entity '{0}'", GetEntity(entity_id)->name);

			}
			else if constexpr (std::is_same<T, PointLightComponent>::value) {
				delete m_point_lights[entity_id];
				m_point_lights[entity_id] = nullptr;
				OAR_CORE_TRACE("Pointlight component deleted from entity '{0}'", GetEntity(entity_id)->name);
			}
			else if constexpr (std::is_same<T, SpotLightComponent>::value) {
				delete m_spot_lights[entity_id];
				m_spot_lights[entity_id] = nullptr;
				OAR_CORE_TRACE("Spotlight component deleted from entity '{0}'", GetEntity(entity_id)->name);
			}
			else if constexpr (std::is_same<T, ScriptComponent>::value) {
				delete m_script_components[entity_id];
				m_script_components[entity_id] = nullptr;
				OAR_CORE_TRACE("Script component deleted from entity '{0}'", GetEntity(entity_id)->name);
			}
		}

		void MakeCameraActive(CameraComponent* p_cam);

		MeshComponent* AddMeshComponent(unsigned long entity_id, const std::string& filename, unsigned int shader_id = 1);
		PointLightComponent* AddPointLightComponent(unsigned long entity_id);
		SpotLightComponent* AddSpotLightComponent(unsigned long entity_id);
		ScriptComponent* AddScriptComponent(unsigned long entity_id);
		CameraComponent* AddCameraComponent(unsigned long entity_id);


		// Creates material and returns ID, get actual material with GetMaterial(id)
		unsigned int CreateMaterial();
		MeshAsset* CreateMeshAsset(const std::string& filename);
		Texture2D* CreateTexture2DAsset(const std::string& filename);

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

		void SortMeshIntoInstanceGroup(MeshComponent* ptr, MeshAsset* asset);
	private:
		void LoadMeshAssetIntoGPU(MeshAsset* asset);
		Texture2D* LoadMeshAssetTexture(const std::string& dir, aiTextureType type, const aiMaterial* material);

		BaseLight m_global_ambient_lighting = BaseLight(0);
		DirectionalLight m_directional_light;

		CameraComponent* mp_active_camera = nullptr;


		std::vector<SceneEntity*> m_entities;
		std::vector<MeshComponent*> m_mesh_components;
		std::vector<SpotLightComponent*> m_spot_lights;
		std::vector<PointLightComponent*> m_point_lights;
		std::vector<ScriptComponent*> m_script_components;
		std::vector<CameraComponent*> m_camera_components;
		std::vector<MeshInstanceGroup*> m_mesh_instance_groups;

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