#include <glew.h>
#include <format>
#include <future>
#include <glfw/glfw3.h>
#include "Scene.h"

Scene::~Scene() {
	UnloadScene();
}


void Scene::LoadScene() {
	double time_start = glfwGetTime();
	for (auto& mesh : m_mesh_data) {
		if (mesh->GetLoadStatus() == false) {
			m_futures.push_back(std::async(std::launch::async, [&mesh, this] {mesh->LoadMeshData(); }));
		}
	}
	for (unsigned int i = 0; i < m_futures.size(); i++) {
		m_futures[i].get();
	}
	for (auto& group : m_mesh_instance_groups) {
		if (group->GetMeshData()->GetLoadStatus() == false) {
			group->GetMeshData()->LoadIntoGL();
			group->InitializeTransformBuffers();
		}
	}
	PrintUtils::PrintSuccess(std::format("All meshes loaded in {}ms", PrintUtils::RoundDouble((glfwGetTime() - time_start) * 1000).substr(0, 4)));
}

void Scene::UnloadScene() {
	PrintUtils::PrintDebug("Unloading scene");
	for (BasicMesh* mesh_data : m_mesh_data) {
		delete mesh_data;
	}
	for (EntityInstanceGroup* group : m_mesh_instance_groups) {
		delete group;
	}
	for (PointLight* light : m_point_lights) {
		delete light;
	}
	PrintUtils::PrintSuccess("Scene unloaded");
}

PointLight* Scene::CreateLight() {
	PointLight* light = new PointLight(glm::fvec3(0.0f, 0.0f, 0.0f), glm::fvec3(1.0f, 1.0f, 1.0f));
	light->SetCubeVisual(CreateMeshEntity("./res/meshes/cube/cube_light.obj", MeshShaderMode::FLAT_COLOR));
	m_point_lights.push_back(light);

	return light;
};

MeshEntity* Scene::CreateMeshEntity(const std::string& filename, MeshShaderMode shader_mode) {
	int group_index = -1;
	int mesh_data_index = -1;

	// check if new entity can merge into already existing instance group
	for (int i = 0; i < m_mesh_instance_groups.size(); i++) {
		//if same data and shader, can be combined so instancing is possible
		if (m_mesh_instance_groups[i]->GetMeshData()->GetFilename() == filename && m_mesh_instance_groups[i]->GetShaderType() == shader_mode) group_index = i;
	}

	//check if mesh data is already available for the instance group about to be created if new entity cannot merge
	if (group_index == -1) {
		for (int i = 0; i < m_mesh_data.size(); i++) {
			if (m_mesh_data[i]->GetFilename() == filename) {
				mesh_data_index = i;
			}
		}

	}

	if (group_index != -1) { // if instance group exists, merge
		PrintUtils::PrintDebug("Instance group found for entity: " + filename);
		auto entity = new MeshEntity(m_mesh_instance_groups[group_index]->GetMeshData());
		m_mesh_instance_groups[group_index]->AddInstance(entity);
		return entity;
	}
	else if (mesh_data_index != -1) { //else if instance group doesn't exist but mesh data exists, create group with existing data
		PrintUtils::PrintDebug("Mesh data found for entity: " + filename);
		auto entity = new MeshEntity(m_mesh_data[mesh_data_index], shader_mode);
		auto group = new EntityInstanceGroup(m_mesh_data[mesh_data_index], shader_mode);
		m_mesh_instance_groups.push_back(group);
		group->AddInstance(entity);
		return entity;
	}
	else { // no instance group, no mesh data -  create mesh data and instance group
		PrintUtils::PrintDebug("Mesh data not found, creating for entity: " + filename);

		BasicMesh* mesh_data = CreateMeshData(filename);
		auto entity = new MeshEntity(mesh_data, shader_mode);
		auto group = new EntityInstanceGroup(mesh_data, shader_mode);
		m_mesh_instance_groups.push_back(group);
		group->AddInstance(entity);

		return entity;
	}
}

BasicMesh* Scene::CreateMeshData(const std::string& filename) {
	for (auto mesh_data : m_mesh_data) {
		if (mesh_data->GetFilename() == filename) {
			PrintUtils::PrintError("MESH DATA ALREADY LOADED IN EXISTING INSTANCE GROUP: " + filename);
			return mesh_data;
		}
	}
	BasicMesh* mesh_data = new BasicMesh(filename);
	m_mesh_data.push_back(mesh_data);
	return mesh_data;
}