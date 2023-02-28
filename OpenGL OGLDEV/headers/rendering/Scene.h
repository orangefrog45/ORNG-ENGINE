#pragma once
#include <memory>
#include <vector>
#include <string>
#include "BasicMesh.h"
#include "MeshEntity.h"
#include "util/util.h"
#include "Light.h"
//TODO : add multiple shader functionality to scene (shadertype member in meshentity probably)
//TODO: add light support

class Scene {
public:
	~Scene();
	void Init();
	void CreateMeshData(const std::string& filename);
	MeshEntity* CreateMeshEntity(unsigned int instances, const std::string& filename);
	BaseLight& GetAmbientLighting() { return m_scene_global_ambient_lighting; };
	void LoadScene();
	void UnloadScene();
	std::vector<MeshEntity*>& GetMeshEntities() { return m_scene_mesh_entities; };
	auto& GetMeshData() { return m_scene_mesh_data; };
private:
	BaseLight m_scene_global_ambient_lighting;
	std::vector <PointLight*> m_scene_lights;
	std::vector<MeshEntity*> m_scene_mesh_entities;
	std::vector<BasicMesh*> m_scene_mesh_data;

};