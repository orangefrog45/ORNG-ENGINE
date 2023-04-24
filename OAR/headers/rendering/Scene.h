#pragma once
#include "rendering/Skybox.h"
#include "terrain/Terrain.h"
#include "components/GridMesh.h"
#include "MeshAsset.h"
#include "components/lights/BaseLight.h"
#include "components/lights/DirectionalLight.h"
#include "components/MeshComponent.h"
#include "components/lights/PointLightComponent.h"
#include "components/lights/SpotLightComponent.h"
#include "components/ScriptComponent.h"
#include "util/Log.h"
#include "rendering/MeshInstanceGroup.h"

class SceneEntity;

class Scene {
public:
	friend class EditorLayer;
	friend class Renderer;
	friend class Component;
	friend class RenderPasses;
	Scene();
	~Scene();
	void Init();

	SceneEntity& CreateEntity(const char* name = "Unnamed entity");

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

		return comp;
	};

	template<std::derived_from<Component> T>
	T* GetComponent(unsigned long entity_id) {

		if constexpr (std::is_same<T, MeshComponent>::value) {
			return m_mesh_components[entity_id];
		}
		else if constexpr (std::is_same<T, PointLightComponent>::value) {
			return m_point_lights[entity_id];
		}
		else if constexpr (std::is_same<T, SpotLightComponent>::value) {
			return m_spot_lights[entity_id];
		}
		else if constexpr (std::is_same<T, ScriptComponent>::value) {
			return m_script_components[entity_id];
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


	MeshComponent* AddMeshComponent(unsigned long entity_id, const std::string& filename, unsigned int shader_id = 0);
	void SortMeshIntoInstanceGroup(MeshComponent* ptr, MeshAsset* asset);
	PointLightComponent* AddPointLightComponent(unsigned long entity_id);
	SpotLightComponent* AddSpotLightComponent(unsigned long entity_id);
	ScriptComponent* AddScriptComponent(unsigned long entity_id);

	MeshAsset* CreateMeshAsset(const std::string& filename);
	Texture2D* CreateTexture2DAsset(const std::string& filename);

	bool LoadMeshAssetIntoGPU(MeshAsset* asset);

	/* Removes asset from all components using it, then deletes asset */
	void DeleteMeshAsset(MeshAsset* data);
	inline auto& GetPointLights() { return m_point_lights; }
	inline auto& GetSpotLights() { return m_spot_lights; }
	inline auto& GetDirectionalLight() { return m_directional_light; }
	inline const auto& GetMeshComponents() const { return m_mesh_components; }
	inline BaseLight& GetAmbientLighting() { return m_global_ambient_lighting; };
	inline auto& GetGroupMeshEntities() { return m_mesh_instance_groups; };
	inline Terrain& GetTerrain() { return m_terrain; }
	inline SceneEntity* GetEntity(unsigned long id) { return m_entities[id]; }

	struct MeshComponentData {
		MeshComponent* component = nullptr;
		unsigned int index;
	};

	void RunUpdateScripts();
	[[nodiscard]] unsigned long CreateEntityID();
	void LoadScene(Camera* cam);
	void UnloadScene();

private:
	Texture2D* LoadMeshAssetTexture(const std::string& dir, aiTextureType type, const aiMaterial* material);

	BaseLight m_global_ambient_lighting = BaseLight(0);
	DirectionalLight m_directional_light;
	std::vector<std::future<void>> m_futures;

	std::vector<SceneEntity*> m_entities;
	std::vector<MeshInstanceGroup*> m_mesh_instance_groups;
	std::vector<MeshComponent*> m_mesh_components;
	std::vector<SpotLightComponent*> m_spot_lights;
	std::vector<PointLightComponent*> m_point_lights;
	std::vector<ScriptComponent*> m_script_components;
	std::vector<MeshAsset*> m_mesh_assets;
	std::vector<Texture2D*> m_texture_2d_assets;

	Terrain m_terrain;
	Skybox m_skybox;
	unsigned long m_last_entity_id = 1; // Last ID assigned to a newly created entity
};