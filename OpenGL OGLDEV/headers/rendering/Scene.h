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
//TODO : add multiple shader functionality to scene (shadertype member in meshentity probably)
//TODO: add light support

class Scene {
public:
	~Scene();
	void Init();
	MeshEntity* CreateMeshEntity(const std::string& filename);
	BaseLight& GetAmbientLighting() { return m_global_ambient_lighting; };
	void LoadScene();
	void UnloadScene();
	auto& GetGroupMeshEntities() { return m_group_mesh_instance_groups; };

private:
	std::vector<std::future<void>> m_futures;
	BasicMesh* CreateMeshData(const std::string& filename);
	BaseLight m_global_ambient_lighting;
	std::vector<EntityInstanceGroup*> m_group_mesh_instance_groups;
	std::vector <PointLight*> m_lights;

};