#include "MeshComponent.h"
#include "MeshInstanceGroup.h"
#include "util/util.h"
#include "WorldTransform.h"

MeshComponent::MeshComponent(MeshData *t_mesh_data, MeshInstanceGroup *instance_group, unsigned int entity_id, MeshData::MeshShaderMode t_shader_mode) : SceneEntity(entity_id), salt_m_mesh_data(t_mesh_data), salt_m_instance_group(instance_group), salt_m_shader_mode(t_shader_mode)
{
	salt_m_transform = new WorldTransform();
};

MeshComponent::~MeshComponent()
{
	delete salt_m_transform;
}

MeshComponent::MeshComponent(const MeshComponent &other) : SceneEntity(other.GetID())
{

	this->salt_m_instance_group = other.GetInstanceGroup();
	this->salt_m_shader_mode = other.salt_m_shader_mode;
	this->salt_color = other.salt_color;
	this->salt_m_mesh_data = other.salt_m_mesh_data;
	this->salt_m_transform = other.salt_m_transform;
	this->SetHierachyValue(other.GetHierachyValue());
}

void MeshComponent::SetPosition(float x, float y, float z)
{
	salt_m_transform->SetPosition(x, y, z);
	UpdateInstanceTransformBuffers();
};

void MeshComponent::SetRotation(float x, float y, float z)
{
	salt_m_transform->SetRotation(x, y, z);
	UpdateInstanceTransformBuffers();
};

void MeshComponent::SetScale(float x, float y, float z)
{
	salt_m_transform->SetScale(x, y, z);
	UpdateInstanceTransformBuffers();
};

void MeshComponent::UpdateInstanceTransformBuffers()
{
	if (salt_m_mesh_data->GetLoadStatus() == true)
		salt_m_instance_group->UpdateMeshTransformBuffers();
}
