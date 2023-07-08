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
		friend class SceneRenderer;
		friend class SceneSerializer;
		Scene();
		~Scene();

		// Only provide uuid if deserializing
		SceneEntity& CreateEntity(const std::string& name, uint64_t uuid = 0);
		SceneEntity* GetEntity(uint64_t uuid);

		void DeleteEntity(SceneEntity* p_entity);

		void Update(float ts);


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
				comp = m_camera_system.AddComponent(p_entity);
			}
			else if constexpr (std::is_same<T, PhysicsComponent>::value) {
				comp = m_physics_system.AddComponent(p_entity, args...);
			}

			return comp;
		};

		template<std::derived_from<Component> T>
		T* GetComponent(uint64_t entity_id) {

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
				return m_camera_system.GetComponent(entity_id);
			}
			else if constexpr (std::is_same<T, TransformComponent>::value) {
				return m_transform_component_manager.GetComponent(entity_id);
			}
			else if constexpr (std::is_same<T, PhysicsComponent>::value) {
				return m_physics_system.GetComponent(entity_id);
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
			else if constexpr (std::is_same<T, CameraComponent>::value) {
				m_camera_system.DeleteComponent(p_entity);
			}
			else if constexpr (std::is_same<T, PhysicsComponent>::value) {
				m_physics_system.DeleteComponent(p_entity);
			}

		}


		MeshComponent* AddMeshComponent(SceneEntity* p_entity, const std::string& filename);
		ScriptComponent* AddScriptComponent(SceneEntity* p_entity);


		Material* CreateMaterial(uint64_t uuid = 0);
		void DeleteMaterial(uint64_t uuid);

		// Specify uuid if deserializing
		MeshAsset* CreateMeshAsset(const std::string& filename, uint64_t uuid = 0);
		// Specify uuid if deserializing
		Texture2D* CreateTexture2DAsset(const Texture2DSpec& spec, uint64_t uuid = 0);

		/* Removes asset from all components using it, then deletes asset */
		void DeleteMeshAsset(MeshAsset* data);

		inline Material* GetMaterial(uint64_t id) const {
			auto it = std::find_if(m_materials.begin(), m_materials.end(), [&](const auto* p_mat) {return p_mat->uuid() == id; });

			if (it == m_materials.end()) {
				OAR_CORE_ERROR("Material with ID '{0}' does not exist, not found", id);
				return nullptr;
			}

			return *it;

		}

		inline Texture2D* GetTexture(uint64_t uuid) {
			auto it = std::find_if(m_texture_2d_assets.begin(), m_texture_2d_assets.end(), [&](const auto* p_tex) {return p_tex->uuid() == uuid; });

			if (it == m_texture_2d_assets.end()) {
				OAR_CORE_WARN("Texture with ID '{0}' does not exist, not found", uuid);
				return nullptr;
			}

			return *it;
		}

		inline MeshAsset* GetMeshAsset(uint64_t uuid) {
			auto it = std::find_if(m_mesh_assets.begin(), m_mesh_assets.end(), [&](const auto* p_tex) {return p_tex->uuid() == uuid; });

			if (it == m_mesh_assets.end()) {
				OAR_CORE_ERROR("Mesh with ID '{0}' does not exist, not found", uuid);
				return nullptr;
			}

			return *it;
		}


		void LoadScene(const std::string& filepath);
		void UnloadScene();


		static inline const uint64_t BASE_MATERIAL_ID = 1;
		static inline const uint64_t DEFAULT_BASE_COLOR_TEX_ID = 1;
	private:
		void LoadMeshAssetIntoGPU(MeshAsset* asset);
		void LoadMeshAssetPreExistingMaterials(MeshAsset* asset, std::vector<Material*>& materials);
		Texture2D* LoadMeshAssetTexture(const std::string& dir, aiTextureType type, const aiMaterial* material);

		BaseLight m_global_ambient_lighting = BaseLight(0);
		DirectionalLight m_directional_light;


		std::vector<SceneEntity*> m_entities;
		std::vector<ScriptComponent*> m_script_components;

		MeshComponentManager m_mesh_component_manager;
		PointlightComponentManager m_pointlight_component_manager;
		SpotlightComponentManager m_spotlight_component_manager;
		TransformComponentManager m_transform_component_manager;
		PhysicsSystem m_physics_system;
		CameraSystem m_camera_system;

		std::vector<Material*> m_materials;
		std::vector<MeshAsset*> m_mesh_assets;
		std::vector<Texture2D*> m_texture_2d_assets;

		Terrain m_terrain;
		Skybox m_skybox;
		GlobalFog m_global_fog;

		std::string m_name = "Untitled scene";

		std::vector<std::future<void>> m_futures;
	};

}