#pragma once
#include <memory>
#include <vector>
#include <string>
#include "BasicMesh.h"
#include "MeshEntity.h"
#include "util/util.h"
//TODO : add multiple shader functionality to scene (shadertype member in meshentity probably)
//TODO: add light support

class Scene {
public:
	void Init();
	void CreateMeshData(const std::string& filename);
	//if filename exists, add and link, if not, createmeshdata with filename
	void CreateMeshEntity(unsigned int instances, const std::string& filename);
	void LoadScene();
	void UnloadScene();
	std::vector<MeshEntity>& GetMeshEntities() { return m_scene_mesh_entities; };
	auto& GetMeshData() { return m_scene_mesh_data; };
private:
	std::vector<MeshEntity> m_scene_mesh_entities;
	std::vector<BasicMesh> m_scene_mesh_data;

};