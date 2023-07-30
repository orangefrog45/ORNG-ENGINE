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
		Scene();
		~Scene();

		void Update(float ts);

		// Specify uuid if deserializing
		SceneEntity& CreateEntity(const std::string& name, uint64_t uuid = 0);
		// Specify uuid if deserializing
		MeshAsset* CreateMeshAsset(const std::string& filename, uint64_t uuid = 0);
		// Specify uuid if deserializing
		Material* CreateMaterial(uint64_t uuid = 0);
		// Specify uuid if deserializing
		Texture2D* CreateTexture2DAsset(const Texture2DSpec& spec, uint64_t uuid = 0);

		/* Removes asset from all components using it, then deletes asset */
		void DeleteMeshAsset(MeshAsset* data);
		/* Removes material from all components using it, then deletes material */
		void DeleteMaterial(uint64_t uuid);
		void DeleteEntity(SceneEntity* p_entity);


		SceneEntity* GetEntity(uint64_t uuid);

		Material* GetMaterial(uint64_t id) const;

		Texture2D* GetTexture(uint64_t uuid);
		MeshAsset* GetMeshAsset(uint64_t uuid);

		Skybox skybox;
		Terrain terrain;
		PostProcessing post_processing;

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

		MeshInstancingSystem m_mesh_component_manager{ &m_registry };
		PointlightSystem m_pointlight_component_manager{ &m_registry };
		SpotlightSystem m_spotlight_component_manager{ &m_registry };
		PhysicsSystem m_physics_system{ &m_registry };
		CameraSystem m_camera_system{ &m_registry };

		entt::registry m_registry;

		std::vector<Material*> m_materials;
		std::vector<MeshAsset*> m_mesh_assets;
		std::vector<Texture2D*> m_texture_2d_assets;


		std::string m_name = "Untitled scene";

		std::vector<std::future<void>> m_futures;
	};

}