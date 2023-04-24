#include "pch/pch.h"

#include "components/MeshComponent.h"
#include "rendering/MeshInstanceGroup.h"
#include "util/util.h"
#include "WorldTransform.h"
#include "rendering/Renderer.h"

MeshComponent::MeshComponent(unsigned long entity_id) : Component(entity_id) { mp_transform = new WorldTransform(); };

MeshComponent::~MeshComponent() {
	delete mp_transform;
}

void MeshComponent::SetPosition(const float x, const float y, const float z) {
	mp_transform->SetPosition(x, y, z);
	UpdateInstanceTransformBuffers();
};

void MeshComponent::SetRotation(const float x, const float y, const float z) {
	mp_transform->SetRotation(x, y, z);
	UpdateInstanceTransformBuffers();
};

void MeshComponent::SetScale(const float x, const float y, const float z) {
	mp_transform->SetScale(x, y, z);
	UpdateInstanceTransformBuffers();
};

void MeshComponent::SetPosition(const glm::vec3 transform) {
	mp_transform->SetPosition(transform.x, transform.y, transform.z);
	UpdateInstanceTransformBuffers();
};

void MeshComponent::SetRotation(const glm::vec3 transform) {
	mp_transform->SetRotation(transform.x, transform.y, transform.z);
	UpdateInstanceTransformBuffers();
};

void MeshComponent::SetScale(const glm::vec3 transform) {
	mp_transform->SetScale(transform.x, transform.y, transform.z);
	UpdateInstanceTransformBuffers();
};

void MeshComponent::SetShaderID(unsigned int id) {
	m_shader_id = id;
	if (mp_mesh_asset)
		mp_instance_group->m_scene->SortMeshIntoInstanceGroup(this, this->mp_mesh_asset);
}

void MeshComponent::UpdateInstanceTransformBuffers() {
	if (mp_mesh_asset && mp_mesh_asset->GetLoadStatus() == true) mp_instance_group->SubUpdateWorldMatBuffer(this);
}


