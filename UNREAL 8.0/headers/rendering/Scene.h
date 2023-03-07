#pragma once
#include <memory>
#include <vector>
#include <string>
#include <future>
#include "BasicMesh.h"
#include "MeshEntity.h"
#include "util/util.h"
#include "Light.h"
#include "EntityInstanceGroup.h"

class Scene {
public:
	~Scene();
	void Init();
	MeshEntity* CreateMeshEntity(const std::string& filename, MeshShaderMode shader_mode = MeshShaderMode::LIGHTING);
	PointLight* CreatePointLight();
	SpotLight* CreateSpotLight();
	auto& GetPointLights() { return m_point_lights; }
	auto& GetSpotLights() { return m_spot_lights; }
	BaseLight& GetAmbientLighting() { return m_global_ambient_lighting; };
	void LoadScene();
	void UnloadScene();
	auto& GetGroupMeshEntities() { return m_mesh_instance_groups; };

private:
	BasicMesh* CreateMeshData(const std::string& filename);
	BaseLight m_global_ambient_lighting;
	std::vector<std::future<void>> m_futures;
	std::vector<EntityInstanceGroup*> m_mesh_instance_groups;
	std::vector<SpotLight*> m_spot_lights;
	std::vector <PointLight*> m_point_lights;
	std::vector<BasicMesh*> m_mesh_data;
};