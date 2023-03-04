#include "MeshEntity.h"
#include "util/util.h"

MeshEntity::MeshEntity(BasicMesh* t_mesh_data) : m_mesh_data(t_mesh_data) {

};

MeshEntity::~MeshEntity() {
	delete m_transform;
}

void MeshEntity::SetPosition(float x, float y, float z) {
	m_transform->SetPosition(x, y, z);
	UpdateInstanceTransformBuffers();
};

void MeshEntity::SetRotation(float x, float y, float z) {
	m_transform->SetRotation(x, y, z);
	UpdateInstanceTransformBuffers();
};

void MeshEntity::SetScale(float x, float y, float z) {
	m_transform->SetScale(x, y, z);
	UpdateInstanceTransformBuffers();
};

void MeshEntity::UpdateInstanceTransformBuffers() {
	m_mesh_data->UpdateTransformBuffers(m_instance_transforms);
}

