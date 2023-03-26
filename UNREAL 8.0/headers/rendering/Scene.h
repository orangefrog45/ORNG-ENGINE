#pragma once
#include <memory>
#include <vector>
#include <string>
#include <future>
#include "MeshData.h"
#include "MeshComponent.h"
#include "util/util.h"
#include "LightComponent.h"
#include "MeshInstanceGroup.h"
#include "terrain/Terrain.h"

class Scene {
public:

	friend class Renderer;
	Scene();
	~Scene();
	void Init();
	MeshComponent& CreateMeshComponent(const std::string& filename, MeshShaderMode shader_mode = MeshShaderMode::LIGHTING);
	PointLightComponent& CreatePointLight();
	SpotLightComponent& CreateSpotLight();

	void DeleteMeshComponent(unsigned int entity_id);
	void DeleteMeshComponent(MeshComponent* ptr);

	inline auto& GetPointLights() { return m_point_lights; }
	inline auto& GetSpotLights() { return m_spot_lights; }
	inline auto& GetDirectionalLight() { return m_directional_light; }
	inline BaseLight& GetAmbientLighting() { return m_global_ambient_lighting; };
	inline auto& GetGroupMeshEntities() { return m_mesh_instance_groups; };
	inline Terrain& GetTerrain() { return m_terrain; }

	const int CreateEntityID();
	void LoadScene();
	void UnloadScene();
	void UpdateEntityInstanceGroups();
	/* check if instance group/mesh component vector requires a resize*/
	void CheckFitsMemory();

private:
	MeshData* CreateMeshData(const std::string& filename);
	BaseLight m_global_ambient_lighting = BaseLight(CreateEntityID());
	DirectionalLightComponent m_directional_light = DirectionalLightComponent(CreateEntityID());
	std::vector<std::future<void>> m_futures;
	std::vector<MeshInstanceGroup> m_mesh_instance_groups;
	std::vector<MeshComponent> m_mesh_components;
	std::vector<SpotLightComponent> m_spot_lights;
	std::vector<PointLightComponent> m_point_lights;
	std::vector<MeshData*> m_mesh_data;
	Terrain m_terrain;
	int m_last_entity_id = -1; // Last ID assigned to a newly created entity
};