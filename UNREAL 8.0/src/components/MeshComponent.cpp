#include "MeshComponent.h"
#include "util/util.h"

MeshComponent::MeshComponent(const MeshComponent& other) : SceneEntity(other.GetID()) {

	this->m_instance_group = other.GetInstanceGroup();
	this->m_shader_mode = other.m_shader_mode;
	this->color = other.color;
	this->m_mesh_data = other.m_mesh_data;
	this->m_transform = other.m_transform;
	this->SetHierachyValue(other.GetHierachyValue());
}

void MeshComponent::SetPosition(float x, float y, float z) {
	m_transform->SetPosition(x, y, z);
	UpdateInstanceTransformBuffers();
};

void MeshComponent::SetRotation(float x, float y, float z) {
	m_transform->SetRotation(x, y, z);
	UpdateInstanceTransformBuffers();
};

void MeshComponent::SetScale(float x, float y, float z) {
	m_transform->SetScale(x, y, z);
	UpdateInstanceTransformBuffers();
};

void MeshComponent::UpdateInstanceTransformBuffers() {
	if (m_mesh_data->GetLoadStatus() == true) m_instance_group->UpdateMeshTransformBuffers();
}


