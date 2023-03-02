#include <future>
#include <glew.h>
#include <freeglut.h>
#include "Scene.h"

Scene::~Scene() {
	UnloadScene();
}

/*void Scene::LoadScene() {
	for (auto& group : m_group_mesh_instance_groups) {
		if (group->m_mesh_data->GetLoadStatus() == false) {
			group->m_mesh_data->LoadMesh();
			group->InitializeTransformBuffers();
		}
	}
}*/


void Scene::LoadScene() {
	int time_start = glutGet(GLUT_ELAPSED_TIME);
	for (auto& group : m_group_mesh_instance_groups) {
		if (group->GetMeshData()->GetLoadStatus() == false) {
			m_futures.push_back(std::async(std::launch::async, [&] {group->GetMeshData()->LoadMeshData(); }));
		}
	}
	for (unsigned int i = 0; i < m_futures.size(); i++) {
		m_futures[i].get();
	}
	for (auto& group : m_group_mesh_instance_groups) {
		if (group->GetMeshData()->GetLoadStatus() == false) {
			group->GetMeshData()->LoadIntoGL();
			group->InitializeTransformBuffers();
		}
	}
	PrintUtils::PrintSuccess("All meshes loaded in " + std::to_string(glutGet(GLUT_ELAPSED_TIME) - time_start));
}

void Scene::UnloadScene() {
	PrintUtils::PrintDebug("Unloading scene");
	for (PointLight* light : m_point_lights) {
		delete light;
	}
	for (auto group : m_group_mesh_instance_groups) {
		delete group;
	}
	PrintUtils::PrintSuccess("Scene unloaded");
}

PointLight* Scene::CreateLight() {
	PointLight* light = new PointLight(glm::fvec3(0.0f, 0.0f, 0.0f), glm::fvec3(1.0f, 1.0f, 1.0f));
	auto light_cube = CreateMeshEntity("./res/meshes/cube/cube_light.obj", MeshShaderMode::FLAT_COLOR);
	light->cube_visual = light_cube;
	m_point_lights.push_back(light);
	return light;
};

MeshEntity* Scene::CreateMeshEntity(const std::string& filename, MeshShaderMode shader_mode) {
	int data_index = -1;
	for (int i = 0; i < m_group_mesh_instance_groups.size(); i++) {
		if (m_group_mesh_instance_groups[i]->GetMeshData()->GetFilename() == filename && m_group_mesh_instance_groups[i]->GetMeshData()->GetShaderMode() == shader_mode) data_index = i;
	}

	if (data_index != -1) {
		MeshEntity* mesh_entity = new MeshEntity(m_group_mesh_instance_groups[data_index]->GetMeshData());
		m_group_mesh_instance_groups[data_index]->AddInstance(mesh_entity);

		PrintUtils::PrintDebug("Mesh data found, loading into entity: " + filename);
		return mesh_entity;
	}
	else {
		BasicMesh* mesh_data = CreateMeshData(filename, shader_mode);
		EntityInstanceGroup* instance_group = new EntityInstanceGroup(mesh_data);
		MeshEntity* mesh_entity = new MeshEntity(mesh_data);

		PrintUtils::PrintDebug("Mesh data not found, creating for entity: " + filename);
		instance_group->AddInstance(mesh_entity);
		m_group_mesh_instance_groups.push_back(instance_group);
		return mesh_entity;
	}
}

BasicMesh* Scene::CreateMeshData(const std::string& filename, MeshShaderMode shader_mode) {
	for (EntityInstanceGroup* group : m_group_mesh_instance_groups) {
		if (group->GetMeshData()->GetFilename() == filename) {
			PrintUtils::PrintError("MESH DATA ALREADY LOADED IN EXISTING INSTANCE GROUP: " + filename);
			return nullptr;
		}
	}

	BasicMesh* mesh_data = new BasicMesh(filename, shader_mode);
	EntityInstanceGroup* group = new EntityInstanceGroup(mesh_data);
	return mesh_data;
}