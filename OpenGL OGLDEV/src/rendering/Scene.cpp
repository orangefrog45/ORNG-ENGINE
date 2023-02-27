#include <algorithm>
#include "Scene.h"

void Scene::LoadScene() {
	for (BasicMesh& mesh : m_scene_mesh_data) {
		mesh.LoadMesh();
		PrintUtils::PrintSuccess("Loaded");
	}
}

void Scene::UnloadScene() {
	for (BasicMesh& mesh : m_scene_mesh_data) {
		mesh.UnloadMesh();
	}
}

void Scene::CreateMeshEntity(unsigned int instances, const std::string& filename) {
	int data_index = -1;
	for (int i = 0; i < m_scene_mesh_data.size(); i++) {
		if (m_scene_mesh_data[i].GetFilename() == filename) data_index = i;
	}

	if (data_index != -1) {
		m_scene_mesh_entities.push_back(MeshEntity(instances, &m_scene_mesh_data[data_index]));
	}
	else {
		CreateMeshData(filename);
		m_scene_mesh_entities.push_back(MeshEntity(instances, &m_scene_mesh_data[m_scene_mesh_data.size() - 1]));
	}
}

void Scene::CreateMeshData(const std::string& filename) {
	if (m_scene_mesh_data.empty()) {
		m_scene_mesh_data.push_back(BasicMesh(filename));
	}
	else {
		for (BasicMesh& mesh : m_scene_mesh_data) {
			if (mesh.GetFilename() == filename) {
				PrintUtils::PrintError("MESH DATA ALREADY LOADED IN SCENE: " + filename);
				return;
			}
			else {
				m_scene_mesh_data.push_back(BasicMesh(filename));
			}
		}

	}

}