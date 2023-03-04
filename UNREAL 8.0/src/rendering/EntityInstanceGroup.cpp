#include "EntityInstanceGroup.h"
#include "util/util.h"

void EntityInstanceGroup::AddInstance(MeshEntity* entity) {
	entity->SetInstanceTransforms(m_transforms);
	m_mesh_entities.push_back(entity);
	m_transforms->push_back(entity->GetWorldTransform());
	m_instances++;
}

void EntityInstanceGroup::InitializeTransformBuffers() {
	m_mesh_data->UpdateTransformBuffers(m_transforms);
}
