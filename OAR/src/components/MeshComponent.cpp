#include "MeshComponent.h"
#include "MeshInstanceGroup.h"
#include "util/util.h"
#include "WorldTransform.h"

MeshComponent::MeshComponent(MeshData* t_mesh_data, MeshInstanceGroup* instance_group, unsigned int entity_id, MeshData::MeshShaderMode t_shader_mode) :
	SceneEntity(entity_id), m_mesh_data(t_mesh_data), m_instance_group(instance_group), m_shader_mode(t_shader_mode)
{
	m_transform = new WorldTransform();
};

MeshComponent::~MeshComponent() {
	delete m_transform;
}

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


