#include "pch/pch.h"

#include "rendering/MeshInstanceGroup.h"
#include "components/WorldTransform.h"
#include "util/Log.h"
#include "scene/Scene.h"

namespace ORNG {

	void MeshInstanceGroup::DeleteMeshPtr(MeshComponent* ptr) {
		int index = FindMeshPtrIndex(ptr);
		m_meshes.erase(m_meshes.begin() + index);
		ptr->mp_instance_group = nullptr;
		m_update_world_mat_buffer_flag = true;
	}

	void MeshInstanceGroup::ClearMeshes() {
		for (int i = 0; i < m_meshes.size(); i++) {
			m_meshes[i]->mp_instance_group = nullptr;
			m_meshes.erase(m_meshes.begin() + i);
		}
	}

	void MeshInstanceGroup::ResortMesh(MeshComponent* ptr) {
		if (m_mesh_asset && m_mesh_asset->is_loaded) {
			DeleteMeshPtr(ptr);
			m_scene->SortMeshIntoInstanceGroup(ptr, ptr->mp_mesh_asset);
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
			transforms.reserve(m_meshes.size() * 0.1); // Rough guess on how many transforms will need to be updated, could be way off, ideally want proper way of estimating this

			// Used for saving the first position of a "chunk" of transforms to be updated, so they can be more efficiently updated with fewer subbuffer calls 
			int first_index_of_chunk = -1;

			for (int i = 0; i < m_meshes.size(); i++) {
				if (m_meshes[i]->m_transform_update_flag)
				{
					m_meshes[i]->m_transform_update_flag = false;
					if (first_index_of_chunk == -1)
						first_index_of_chunk = i;

					transforms.push_back(glm::transpose(m_meshes[i]->GetWorldTransform()->GetMatrix()));
				}
				else if (first_index_of_chunk != -1) // This is the end of the chunk, so update for this chunk 
				{
					m_mesh_asset->m_vao.SubUpdateTransformSSBO(m_transform_ssbo_handle, first_index_of_chunk, transforms);
					transforms.clear();
					transforms.reserve((m_meshes.size() - i) * 0.1);
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
		m_meshes.push_back(ptr);

		m_update_world_mat_buffer_flag = true;
	}

	void MeshInstanceGroup::UpdateTransformSSBO() {
		if (!m_mesh_asset->GetLoadStatus() || m_meshes.empty()) {
			return;
		}

		std::vector<glm::mat4> transforms;
		transforms.reserve(m_meshes.size());

		for (auto& p_mesh : m_meshes) {
			transforms.emplace_back(glm::transpose(p_mesh->GetWorldTransform()->GetMatrix()));
		}

		m_mesh_asset->m_vao.FullUpdateTransformSSBO(m_transform_ssbo_handle, &transforms);

	}

	int MeshInstanceGroup::FindMeshPtrIndex(const MeshComponent* ptr) {
		auto it = std::find(m_meshes.begin(), m_meshes.end(), ptr);

		if (it == m_meshes.end()) {
			OAR_CORE_ERROR("Mesh component ptr not found in instance group with asset '{0}'\nShader '{1}'\nMaterial'{2}'", m_mesh_asset->m_filename, m_group_shader_id, m_group_material_id);
			return -1;
		}

		return it - m_meshes.begin();
	}

}