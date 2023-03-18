#include <glew.h>
#include <format>
#include <future>
#include <glfw/glfw3.h>
#include "Scene.h"
#include "RendererData.h"

Scene::~Scene() {
	UnloadScene();
}

Scene::Scene() {
	m_spot_lights.reserve(RendererData::max_spot_lights);
	m_point_lights.reserve(RendererData::max_point_lights);
	m_mesh_instance_groups.reserve(10);
	m_mesh_components.reserve(100);
}

void Scene::DeleteMeshComponent(unsigned int entity_id) {
	for (auto& mesh : m_mesh_components) {
		if (mesh.GetID() == entity_id) {
			delete mesh.GetWorldTransform();
			mesh.~MeshComponent();
			mesh.GetInstanceGroup()->ValidateTransformPtrs();
		}
	}
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
		if (group.GetMeshData()->GetLoadStatus() == false) {
			group.GetMeshData()->LoadIntoGL();
			group.InitializeTransformBuffers();
		}
	}
	PrintUtils::PrintSuccess(std::format("Scene loaded in {}ms", PrintUtils::RoundDouble((glfwGetTime() - time_start) * 1000)));
}


void Scene::UnloadScene() {
	PrintUtils::PrintDebug("Unloading scene");
	for (MeshData* mesh_data : m_mesh_data) {
		delete mesh_data;
	}
	PrintUtils::PrintSuccess("Scene unloaded");
}


PointLightComponent& Scene::CreatePointLight() {
	if (m_point_lights.size() + 1 > m_point_lights.capacity()) {
		PrintUtils::PrintError("CANNOT CREATE POINTLIGHT, LIMIT EXCEEDED");
	}
	else {
		m_point_lights.emplace_back(CreateEntityID());
		m_point_lights.back().SetMeshVisual(&CreateMeshComponent("./res/meshes/light meshes/cube_light.obj", MeshShaderMode::FLAT_COLOR));
	}

	return m_point_lights.back();
};


SpotLightComponent& Scene::CreateSpotLight() {
	if (m_spot_lights.size() + 1 > m_spot_lights.capacity()) {
		PrintUtils::PrintError("CANNOT CREATE SPOTLIGHT, LIMIT EXCEEDED");
	}
	else {
		m_spot_lights.emplace_back(CreateEntityID());
		m_spot_lights.back().SetMeshVisual(&CreateMeshComponent("./res/meshes/light meshes/cone.obj", MeshShaderMode::FLAT_COLOR));

	}
	return m_spot_lights.back();
}

void Scene::UpdateEntityInstanceGroups() {
	m_mesh_instance_groups.reserve(m_mesh_components.capacity() * 2);

	for (auto& mesh : m_mesh_components) {
		for (auto& group : m_mesh_instance_groups) {
			if (group.GetShaderType() == mesh.GetShaderMode() && group.GetMeshData()->GetFilename() == mesh.GetMeshData()->GetFilename()) {
				mesh.SetInstanceGroup(group);
				break;
			}
		}
	}
}

void Scene::CheckFitsMemory() {

	if (m_mesh_components.size() + 1 == m_mesh_components.capacity()) {
		PrintUtils::PrintDebug("MESH COMPONENT ARRAY ALLOCATING NEW SPACE");

		/* Light mesh visuals require pointers to entities, upon invalidation of pointers (vector resize), renew them */
		if (!m_spot_lights.empty()) {

			m_mesh_components.reserve(m_mesh_components.capacity() * 2);

			/* Search for pointer to newly created mesh visual for spot lights */
			for (auto& spot_light : m_spot_lights)
			{
				unsigned int spot_light_mesh_component_id = spot_light.GetMeshVisual()->GetID();
				MeshComponent* mesh_ptr = nullptr;
				for (auto& mesh : m_mesh_components) {
					if (mesh.GetID() == spot_light_mesh_component_id) {
						mesh_ptr = &mesh;
						break;
					}
				}
				spot_light.SetMeshVisual(mesh_ptr);
			}

			for (auto& point_light : m_spot_lights) {
				unsigned int spot_light_mesh_component_id = point_light.GetMeshVisual()->GetID();
				MeshComponent* mesh_ptr = nullptr;
				for (auto& mesh : m_mesh_components) {
					if (mesh.GetID() == spot_light_mesh_component_id) {
						mesh_ptr = &mesh;
						break;
					}
				}
				point_light.SetMeshVisual(mesh_ptr);
			}

		}
	}

	if (m_mesh_instance_groups.size() + 1 == m_mesh_instance_groups.capacity()) {
		PrintUtils::PrintDebug("INSTANCE GROUP ARRAY ALLOCATING NEW SPACE");
		m_mesh_instance_groups.reserve(m_mesh_instance_groups.capacity() * 2);
		UpdateEntityInstanceGroups();
	}

}


MeshComponent& Scene::CreateMeshComponent(const std::string& filename, MeshShaderMode shader_mode) {
	int group_index = -1;
	int mesh_data_index = -1;

	CheckFitsMemory();

	// check if new entity can merge into already existing instance group
	for (int i = 0; i < m_mesh_instance_groups.size(); i++) {
		//if same data and shader, can be combined so instancing is possible
		if (m_mesh_instance_groups[i].GetMeshData()->GetFilename() == filename && m_mesh_instance_groups[i].GetShaderType() == shader_mode) {
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
		mesh_component = &m_mesh_components.emplace_back(m_mesh_instance_groups[group_index].GetMeshData(), &m_mesh_instance_groups[group_index], CreateEntityID(), shader_mode);

		// add mesh component's world transform into instance group for instance rendering
		m_mesh_instance_groups[group_index].AddTransformPtr(mesh_component->GetWorldTransform());

	}
	else if (mesh_data_index != -1) { //else if instance group doesn't exist but mesh data exists, create group with existing data
		PrintUtils::PrintDebug("Mesh data found for entity: " + filename);
		auto& group = m_mesh_instance_groups.emplace_back(m_mesh_data[mesh_data_index], shader_mode); //create new instance group
		group.GetMeshData()->SetIsShared(true); // Required if mesh data is shared as the transform buffers will be too, requiring per-shader transform updates
		mesh_component = &m_mesh_components.emplace_back(m_mesh_data[mesh_data_index], &group, CreateEntityID(), shader_mode); //create new mesh component, place into new instance group
		group.AddTransformPtr(mesh_component->GetWorldTransform());
	}
	else { // no instance group, no mesh data -  create mesh data and instance group
		PrintUtils::PrintDebug("Mesh data not found, creating for entity: " + filename);

		MeshData* mesh_data = CreateMeshData(filename); // create mesh data
		auto& group = m_mesh_instance_groups.emplace_back(mesh_data, shader_mode);
		mesh_component = &m_mesh_components.emplace_back(mesh_data, &group, CreateEntityID(), shader_mode);
		group.AddTransformPtr(mesh_component->GetWorldTransform());
	}

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
