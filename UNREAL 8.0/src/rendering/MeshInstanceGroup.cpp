#include "MeshInstanceGroup.h"

void MeshInstanceGroup::ValidateTransformPtrs() {
	for (unsigned int i = 0; i < m_transforms.size(); i++) {
		if (m_transforms[i] == nullptr) {
			m_transforms.erase(m_transforms.begin() + i);
		}
	}
}

void MeshInstanceGroup::InitializeTransformBuffers() {
	m_mesh_data->UpdateTransformBuffers(m_transforms);
}
