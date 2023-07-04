#include "pch/pch.h"

#include "scene/MeshInstanceGroup.h"
#include "components/ComponentManagers.h"
#include "components/TransformComponent.h"
#include "util/Log.h"
#include "scene/Scene.h"
#include "scene/SceneEntity.h"

namespace ORNG {

	void MeshInstanceGroup::DeleteMeshPtr(MeshComponent* ptr) {
		int index = FindMeshPtrIndex(ptr);
		m_instances.erase(m_instances.begin() + index);
		ptr->mp_instance_group = nullptr;
		m_update_world_mat_buffer_flag = true;
	}

	void MeshInstanceGroup::ClearMeshes() {
		for (int i = 0; i < m_instances.size(); i++) {
			m_instances[i]->mp_instance_group = nullptr;
			m_instances.erase(m_instances.begin() + i);
		}
	}

	void MeshInstanceGroup::ResortMesh(MeshComponent* ptr) {
		if (m_mesh_asset && m_mesh_asset->m_is_loaded) {
			DeleteMeshPtr(ptr);
			mp_manager->SortMeshIntoInstanceGroup(ptr, ptr->mp_mesh_asset);
		}
	}

	void MeshInstanceGroup::ProcessUpdates() {

		// If the whole buffer needs to be updated, just do that, ignore sub updates as they'll happen anyway in the full buffer update 
		if (m_update_world_mat_buffer_flag) {
			m_update_world_mat_buffer_flag = false;
			UpdateTransformSSBO();
		}
		else if (m_sub_update_world_mat_buffer_flag) {
			m_sub_update_world_mat_buffer_flag = false;
			std::vector<glm::mat4> transforms;
			transforms.reserve(m_instances.size() * 0.1); // Rough guess on how many transforms will need to be updated, could be way off, ideally want proper way of estimating this

			// Used for saving the first position of a "chunk" of transforms to be updated, so they can be more efficiently updated with fewer subbuffer calls 
			int first_index_of_chunk = -1;

			for (int i = 0; i < m_instances.size(); i++) {
				if (m_instances[i]->m_transform_update_flag)
				{
					m_instances[i]->m_transform_update_flag = false;
					if (first_index_of_chunk == -1)
						first_index_of_chunk = i;

					transforms.push_back(m_instances[i]->p_transform->GetMatrix());
				}
				else if (first_index_of_chunk != -1) // This is the end of the chunk, so update for this chunk 
				{
					m_mesh_asset->m_vao.SubUpdateTransformSSBO(m_transform_ssbo_handle, first_index_of_chunk, transforms);
					transforms.clear();
					transforms.reserve((m_instances.size() - i) * 0.1);
					first_index_of_chunk = -1;
				}
			}

			// Last check, if the chunk persisted until the end of the array 
			if (first_index_of_chunk != -1) {
				m_mesh_asset->m_vao.SubUpdateTransformSSBO(m_transform_ssbo_handle, first_index_of_chunk, transforms);
			}

		}
	}

	void MeshInstanceGroup::AddMeshPtr(MeshComponent* ptr) {
		ptr->mp_instance_group = this;
		ptr->mp_mesh_asset = m_mesh_asset;
		m_instances.push_back(ptr);

		m_update_world_mat_buffer_flag = true;
	}

	void MeshInstanceGroup::UpdateTransformSSBO() {
		if (!m_mesh_asset->GetLoadStatus() || m_instances.empty()) {
			return;
		}

		std::vector<glm::mat4> transforms;
		transforms.reserve(m_instances.size());

		for (auto& p_mesh : m_instances) {
			transforms.emplace_back(p_mesh->p_transform->GetMatrix());
		}

		m_mesh_asset->m_vao.FullUpdateTransformSSBO(m_transform_ssbo_handle, &transforms);

	}

	int MeshInstanceGroup::FindMeshPtrIndex(const MeshComponent* ptr) {
		auto it = std::find(m_instances.begin(), m_instances.end(), ptr);

		if (it == m_instances.end()) {
			OAR_CORE_ERROR("Mesh component ptr not found in instance group with asset '{0}'\nShader '{1}'", m_mesh_asset->m_filename, m_group_shader_id);
			return -1;
		}

		return it - m_instances.begin();
	}

}