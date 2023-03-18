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

class Scene {
public:
	Scene();
	~Scene();
	void Init();
	MeshComponent& CreateMeshComponent(const std::string& filename, MeshShaderMode shader_mode = MeshShaderMode::LIGHTING);
	PointLightComponent& CreatePointLight();
	SpotLightComponent& CreateSpotLight();
	void DeleteMeshComponent(unsigned int entity_id);
	auto& GetPointLights() { return m_point_lights; }
	auto& GetSpotLights() { return m_spot_lights; }
	auto& GetDirectionalLight() { return m_directional_light; }
	const int CreateEntityID();
	BaseLight& GetAmbientLighting() { return m_global_ambient_lighting; };
	void LoadScene();
	void UnloadScene();
	void UpdateEntityInstanceGroups();
	/* check if instance group/mesh component vector requires a resize*/
	void CheckFitsMemory();
	auto& GetGroupMeshEntities() { return m_mesh_instance_groups; };

private:
	int m_last_group_id = -1;
	int m_last_entity_id = -1; // Last ID assigned to a newly created entity
	MeshData* CreateMeshData(const std::string& filename);
	BaseLight m_global_ambient_lighting = BaseLight(CreateEntityID());
	DirectionalLightComponent m_directional_light = DirectionalLightComponent(CreateEntityID());
	std::vector<std::future<void>> m_futures;
	std::vector<MeshInstanceGroup> m_mesh_instance_groups;
	std::vector<MeshComponent> m_mesh_components;
	std::vector<SpotLightComponent> m_spot_lights;
	std::vector<PointLightComponent> m_point_lights;
	std::vector<MeshData*> m_mesh_data;
};