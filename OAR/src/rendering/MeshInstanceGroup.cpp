#include "pch/pch.h"

#include <glm/gtx/matrix_major_storage.hpp>
#include "rendering/MeshInstanceGroup.h"
#include "WorldTransform.h"
#include "util/Log.h"

void MeshInstanceGroup::DeleteMeshPtr(const MeshComponent* ptr) {
	int index = FindMeshPtrIndex(ptr);
	m_meshes.erase(m_meshes.begin() + index);
	UpdateWorldMatBuffer();
}

void MeshInstanceGroup::AddMeshPtr(MeshComponent* ptr) {

	m_meshes.push_back(ptr);
	UpdateWorldMatBuffer();


	//Code currently not needed, newly created meshes always have higher id's so auto-sort themselves. When shader switching during runtime, will need this.

	/*unsigned int low = 0;
	unsigned int high = m_meshes.size() - 1;
	unsigned int index = (low + high) / 2;

	/* Sort mesh component by id into vector */
	/*while (true) {

		if (m_meshes[index]->GetEntityHandle() > ptr->GetEntityHandle()) {
			if (m_meshes[index - 1]->GetEntityHandle() < ptr->GetEntityHandle()) {
				m_meshes.insert(m_meshes.begin() + index - 1, ptr);
				UpdateWorldMatBuffer();
				break;
			}

			high = index;
		}
		else if (m_meshes[index]->GetEntityHandle() < ptr->GetEntityHandle()) {
			low = index;
		}

		index = (low + high) / 2;
	}*/
}

void MeshInstanceGroup::UpdateWorldMatBuffer() {
	if (!m_mesh_asset->GetLoadStatus()) {
		return;
	}

	static std::vector<glm::mat4> transforms;
	transforms.reserve(m_meshes.size());

	for (auto& p_mesh : m_meshes) {
		transforms.emplace_back(glm::rowMajor4(p_mesh->GetWorldTransform()->GetMatrix()));
	}

	m_mesh_asset->UpdateWorldTransformBuffer(transforms);
}

unsigned int MeshInstanceGroup::FindMeshPtrIndex(const MeshComponent* ptr) {
	return std::find(m_meshes.begin(), m_meshes.end(), ptr) - m_meshes.begin();
}

void MeshInstanceGroup::SubUpdateWorldMatBuffer(const MeshComponent* ptr) {

	if (m_meshes.empty()) {
		OAR_CORE_ERROR("Transform buffer update failed for entity with ID '{0}', entity not in instance group", ptr->GetEntityHandle());
		return;
	}
	unsigned int index = FindMeshPtrIndex(ptr);

	m_mesh_asset->SubUpdateWorldTransformBuffer(index, glm::rowMajor4(m_meshes[index]->GetWorldTransform()->GetMatrix()));
}
