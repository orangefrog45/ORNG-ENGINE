#include "EntityInstanceGroup.h"
#include "util/util.h"

void EntityInstanceGroup::AddInstance(MeshEntity* entity) {
	entity->SetInstanceTransforms(m_transforms);
	m_transforms->push_back(entity->GetWorldTransform());
	m_mesh_entities.push_back(entity);
	m_instances++;
	m_mesh_data->UpdateTransformBuffers(m_transforms);
}

EntityInstanceGroup::~EntityInstanceGroup() {
	for (auto mesh : m_mesh_entities) {
		delete mesh;
	}

	delete m_transforms;
	delete m_mesh_data;
}
/*void EntityInstanceGroup::DeleteInstance(unsigned int index) {
	if (index < m_transform_matrices.size()) {
		//TODO: MAKE THIS SUBBUFFER UPDATE TRANSFORM BUFFERS
		m_transform_matrices.erase(m_transform_matrices.begin() + index);
		m_mesh_data->UpdateTransformBuffers(m_transform_matrices);
		PrintUtils::PrintDebug("Instance of '" + m_mesh_data->GetFilename() + "' deleted");
	}
	else {
		PrintUtils::PrintError("No instance of '" + m_mesh_data->GetFilename() + "' found");
		return;

	}

}


void EntityInstanceGroup::ModifyInstance(unsigned int index, glm::fmat4& transform) {
	if (index < m_transform_matrices.size()) {
		m_transform_matrices[index] = transform;
		m_mesh_data->UpdateTransformBuffers(m_transform_matrices);
	}
	else {
		PrintUtils::PrintError("No instance of '" + m_mesh_data->GetFilename() + "' found");
	}
}*/
