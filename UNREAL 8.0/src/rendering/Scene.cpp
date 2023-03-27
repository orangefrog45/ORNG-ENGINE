#include <glew.h>
#include <format>
#include <glfw/glfw3.h>
#include "Scene.h"
#include "util/util.h"

Scene::~Scene() {
	UnloadScene();
}

Scene::Scene() {
	m_spot_lights.reserve(RendererData::max_spot_lights);
	m_point_lights.reserve(RendererData::max_point_lights);
	m_mesh_instance_groups.reserve(20);
	m_mesh_components.reserve(100);
}

void Scene::DeleteMeshComponent(unsigned int entity_id) {
	MeshComponentData data = QueryMeshComponent(entity_id);
	unsigned int index = data.index;
	auto mesh = data.component;
	auto group = mesh->GetInstanceGroup();
	delete mesh;
	group->ValidateTransformPtrs();
	m_mesh_components.erase(m_mesh_components.begin() + index);
}

void Scene::DeleteMeshComponent(MeshComponent* ptr) {
	unsigned int index = QueryMeshComponent(ptr->GetID()).index;
	auto group = ptr->GetInstanceGroup();
	delete ptr;
	ptr->GetInstanceGroup()->ValidateTransformPtrs();
	m_mesh_components.erase(m_mesh_components.begin() + index);
}

Scene::MeshComponentData Scene::QueryMeshComponent(unsigned int id) {
	MeshComponentData data;
	size_t index = (m_mesh_components.size() - 1) / 2;;
	int low = 0;
	int high = m_mesh_components.size() - 1;

	while (m_mesh_components[index]->GetID() != id) {
		if (m_mesh_components[index]->GetID() < id) {
			low = index + 1;
			index = (high + low) / 2;
		}
		else if (m_mesh_components[index]->GetID() > id) {
			high = index - 1;
			index = (high + low) / 2;
		}
	}

	data.index = index;
	data.component = m_mesh_components[index];

	return data;
}



void Scene::LoadScene() {
	double time_start = glfwGetTime();

	m_terrain.Init();

	for (auto& mesh : m_mesh_data) {
		if (mesh->GetLoadStatus() == false) {
			m_futures.push_back(std::async(std::launch::async, [&mesh] {mesh->LoadMeshData(); }));
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
	PrintUtils::PrintSuccess(std::format("Scene loaded in {}ms", PrintUtils::RoundDouble((glfwGetTime() - time_start) * 1000)));
}


void Scene::UnloadScene() {
	PrintUtils::PrintDebug("Unloading scene");
	for (MeshData* mesh_data : m_mesh_data) {
		delete mesh_data;
	}
	for (auto& mesh : m_mesh_components) {
		delete mesh;
	}
	for (auto& group : m_mesh_instance_groups) {
		delete group;
	}
	for (auto& light : m_point_lights) {
		delete light;
	}
	for (auto& light : m_spot_lights) {
		delete light;
	}

	PrintUtils::PrintSuccess("Scene unloaded");
}


PointLightComponent& Scene::CreatePointLight() {
	PointLightComponent* light = nullptr;
	if (m_point_lights.size() + 1 > m_point_lights.capacity()) {
		PrintUtils::PrintError("CANNOT CREATE POINTLIGHT, LIMIT EXCEEDED");
	}
	else {
		light = new PointLightComponent(CreateEntityID());
		m_point_lights.push_back(light);
		light->SetMeshVisual(&CreateMeshComponent("./res/meshes/light meshes/cube_light.obj", MeshData::MeshShaderMode::FLAT_COLOR));
	}

	return *light;
};


SpotLightComponent& Scene::CreateSpotLight() {
	SpotLightComponent* light = nullptr;
	if (m_spot_lights.size() + 1 > m_spot_lights.capacity()) {
		PrintUtils::PrintError("CANNOT CREATE SPOTLIGHT, LIMIT EXCEEDED");
	}
	else {
		light = new SpotLightComponent(CreateEntityID());
		m_spot_lights.push_back(light);
		light->SetMeshVisual(&CreateMeshComponent("./res/meshes/light meshes/cone.obj", MeshData::MeshShaderMode::FLAT_COLOR));
	}
	return *light;
}


MeshComponent& Scene::CreateMeshComponent(const std::string& filename, MeshData::MeshShaderMode shader_mode) {
	int group_index = -1;
	int mesh_data_index = -1;

	// check if new entity can merge into already existing instance group
	for (int i = 0; i < m_mesh_instance_groups.size(); i++) {
		//if same data and shader, can be combined so instancing is possible
		if (m_mesh_instance_groups[i]->GetMeshData()->GetFilename() == filename && m_mesh_instance_groups[i]->GetShaderType() == shader_mode) {
			group_index = i;
			break;
		}
	}

	//check if mesh data is already available for the instance group about to be created if new entity cannot merge
	if (group_index == -1) {
		for (int i = 0; i < m_mesh_data.size(); i++) {
			if (m_mesh_data[i]->GetFilename() == filename) {
				mesh_data_index = i;
				break;
			}
		}

	}

	MeshComponent* mesh_component = nullptr;

	if (group_index != -1) { // if instance group exists, merge
		PrintUtils::PrintDebug("Instance group found for entity: " + filename);

		//place mesh component in existing instance group
		mesh_component = new MeshComponent(m_mesh_instance_groups[group_index]->GetMeshData(), m_mesh_instance_groups[group_index], CreateEntityID(), shader_mode);
		// add mesh component's world transform into instance group for instanced rendering
		m_mesh_instance_groups[group_index]->AddTransformPtr(mesh_component->GetWorldTransform());

	}
	else if (mesh_data_index != -1) { //else if instance group doesn't exist but mesh data exists, create group with existing data
		PrintUtils::PrintDebug("Mesh data found for entity: " + filename);

		auto group = new MeshInstanceGroup(m_mesh_data[mesh_data_index], shader_mode);
		m_mesh_instance_groups.push_back(group);
		group->GetMeshData()->SetIsShared(true); // Required if mesh data is shared as the transform buffers will be too, requiring per-shader transform updates
		mesh_component = new MeshComponent(m_mesh_data[mesh_data_index], group, CreateEntityID(), shader_mode);
		group->AddTransformPtr(mesh_component->GetWorldTransform());
	}
	else { // no instance group, no mesh data -  create mesh data and instance group
		PrintUtils::PrintDebug("Mesh data not found, creating for entity: " + filename);

		MeshData* mesh_data = CreateMeshData(filename); // create mesh data
		auto group = new MeshInstanceGroup(mesh_data, shader_mode);
		m_mesh_instance_groups.push_back(group);
		mesh_component = new MeshComponent(mesh_data, group, CreateEntityID(), shader_mode);
		group->AddTransformPtr(mesh_component->GetWorldTransform());
	}

	m_mesh_components.push_back(mesh_component);
	return *mesh_component;

}

MeshData* Scene::CreateMeshData(const std::string& filename) {
	for (auto mesh_data : m_mesh_data) {
		if (mesh_data->GetFilename() == filename) {
			PrintUtils::PrintError("MESH DATA ALREADY LOADED IN EXISTING INSTANCE GROUP: " + filename);
			return mesh_data;
		}
	}
	MeshData* mesh_data = new MeshData(filename);
	m_mesh_data.push_back(mesh_data);
	return mesh_data;
}


const int Scene::CreateEntityID() {
	m_last_entity_id++;
	return m_last_entity_id;
}
