#include <algorithm>
#include "Scene.h"

Scene::~Scene() {
	UnloadScene();
}

void Scene::LoadScene() {
	for (BasicMesh* mesh : m_scene_mesh_data) {
		mesh->LoadMesh();
	}
}

void Scene::UnloadScene() {
	PrintUtils::PrintDebug("Unloading scene");
	for (BasicMesh* mesh : m_scene_mesh_data) {
		mesh->UnloadMesh();
		delete mesh;
	}
	for (MeshEntity* entity : m_scene_mesh_entities) {
		delete entity;
	}
	for (PointLight* light : m_scene_lights) {
		delete light;
	}
	PrintUtils::PrintSuccess("Scene unloaded");
}

MeshEntity* Scene::CreateMeshEntity(unsigned int instances, const std::string& filename) {
	int data_index = -1;
	for (int i = 0; i < m_scene_mesh_data.size(); i++) {
		if (m_scene_mesh_data[i]->GetFilename() == filename) data_index = i;
	}

	if (data_index != -1) {
		m_scene_mesh_entities.emplace_back(new MeshEntity(instances, m_scene_mesh_data[data_index]));
		PrintUtils::PrintDebug("Mesh data found, loading into entity: " + filename);
		return m_scene_mesh_entities[m_scene_mesh_entities.size() - 1];
	}
	else {
		CreateMeshData(filename);
		m_scene_mesh_entities.emplace_back(new MeshEntity(instances, m_scene_mesh_data[m_scene_mesh_data.size() - 1]));
		PrintUtils::PrintDebug("Mesh data not found, creating for entity: " + filename);
		return m_scene_mesh_entities[m_scene_mesh_entities.size() - 1];
	}
}

void Scene::CreateMeshData(const std::string& filename) {
	if (m_scene_mesh_data.empty()) {
		m_scene_mesh_data.emplace_back(new BasicMesh(filename));
	}
	else {
		for (BasicMesh* mesh : m_scene_mesh_data) {
			if (mesh->GetFilename() == filename) {
				PrintUtils::PrintError("MESH DATA ALREADY LOADED IN SCENE: " + filename);
				return;
			}
			else {
				m_scene_mesh_data.emplace_back(new BasicMesh(filename));
			}
		}

	}

}