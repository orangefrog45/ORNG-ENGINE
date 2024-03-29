#include "pch/pch.h"

#include "scene/MeshInstanceGroup.h"
#include "components/TransformComponent.h"
#include "scene/SceneEntity.h"
#include "glm/glm/gtc/round.hpp"
#include "rendering/MeshAsset.h"

namespace ORNG {
	MeshInstanceGroup::MeshInstanceGroup(MeshAsset* t_mesh_data, MeshInstancingSystem* p_mcm, const std::vector<const Material*>& materials, entt::registry& registry) :
		m_mesh_asset(t_mesh_data), m_materials(materials), m_registry(registry)
	{
		// Setup a transform matrix ssbo for this instance group
		m_transform_ssbo.Init();

		// Max of 10k wasted vertices rendered (unlikely unless frequently removing instances)
		// Check the vertex data too in case the mesh hasn't fully loaded yet (GetIndicesCount will return 0)
		m_tombstone_limit = glm::max(10000 / (int)glm::max(m_mesh_asset->GetIndicesCount(), (unsigned)m_mesh_asset->m_vao.vertex_data.indices.size()), 1); 
	};

	void MeshInstanceGroup::RemoveInstance(SceneEntity* ptr) {
		auto entt_handle = ptr->GetEnttHandle();

		if (!m_instances.contains(entt_handle)) {
			auto idx = VectorFindIndex(m_entities_to_instance, entt_handle);
			m_entities_to_instance.erase(m_entities_to_instance.begin() + idx);
			return;
		}

		glm::mat4 tombstone_transform = glm::scale(glm::vec3(0));
		glNamedBufferSubData(m_transform_ssbo.GetHandle(), m_instances[entt_handle] * sizeof(glm::mat4), sizeof(glm::mat4), &tombstone_transform[0][0]);
		m_instances.erase(entt_handle);
		std::erase_if(m_instances_to_update, [entt_handle](entt::entity entity) {return entt_handle == entity; });
		m_tombstone_count++;

		if (m_tombstone_count >= m_tombstone_limit) {
			ReallocateInstances();
		}


	}

	void MeshInstanceGroup::ClearMeshes() {
		while (!m_instances.empty()) {
			m_instances.erase(m_instances.begin());
		}
	}


	void MeshInstanceGroup::FlagInstanceTransformUpdate(SceneEntity* p_instance) {
		m_instances_to_update.push_back(p_instance->GetEnttHandle());
	}


	void MeshInstanceGroup::ProcessUpdates() {

		if (m_transform_ssbo.GetGPU_BufferSize() == 0)
			m_transform_ssbo.Resize(64);

		if (!m_entities_to_instance.empty()) {
			ORNG_TRACY_PROFILE;

			// Check if transform buffer is big enough for new transforms
			unsigned transform_buf_size = m_transform_ssbo.GetGPU_BufferSize();
			unsigned min_memory_required = (m_instances.size() + m_tombstone_count + m_entities_to_instance.size()) * sizeof(glm::mat4);
			if (transform_buf_size <= min_memory_required) {
				m_transform_ssbo.Resize(glm::roundMultiple(min_memory_required * 3 / 2, (unsigned)sizeof(glm::mat4)));
			}

			// Append transforms to end of buffer
			{
				std::vector<std::byte> transform_buf{ m_entities_to_instance.size() * sizeof(glm::mat4) };
				std::byte* p_byte = transform_buf.data();

				auto prev_used_transform_memory_end_idx = m_used_transform_memory_end_idx;
				for (auto entt_handle : m_entities_to_instance) {
					m_instances[entt_handle] = m_used_transform_memory_end_idx;
					m_used_transform_memory_end_idx++; // m_used_transform_memory_end_idx will only decrease when the buffer is reallocated
					ConvertToBytes(p_byte, m_registry.get<TransformComponent>(entt_handle).GetMatrix());
				}

				glNamedBufferSubData(m_transform_ssbo.GetHandle(), prev_used_transform_memory_end_idx * sizeof(glm::mat4), transform_buf.size(),
					transform_buf.data());

			}

			m_entities_to_instance.clear();
		}


		if (m_instances_to_update.empty() || m_instances.empty())
			return;

		// Sort so neighbouring transforms can be found and updated all in one call to buffersubdata
		std::ranges::sort(m_instances_to_update, [this](const auto& p_ent, const auto& p_ent2) {return m_instances[p_ent] < m_instances[p_ent2]; });

		// Erase duplicate instances flagged for update
		m_instances_to_update.erase(std::unique(m_instances_to_update.begin(), m_instances_to_update.end()), m_instances_to_update.end());

		std::vector<glm::mat4> transforms;
		transforms.reserve(m_instances_to_update.size());

		// Used for saving the first position of a "chunk" of transforms to be updated, so they can be more efficiently updated with fewer buffersubdata calls
		int first_index_of_chunk = -1;

		// Below loop can't cover index 0 (first check puts it out of range) so handle separately here
		transforms.push_back(m_registry.get<TransformComponent>(m_instances_to_update[0]).GetMatrix());
		glNamedBufferSubData(m_transform_ssbo.GetHandle(), m_instances[m_instances_to_update[0]] * sizeof(glm::mat4), transforms.size() * sizeof(glm::mat4), &transforms[0]);
		transforms.clear();
		
		for (int i = 1; i < m_instances_to_update.size(); i++) {
			if (m_instances[m_instances_to_update[i]] == m_instances[m_instances_to_update[i - 1]] + 1) // Check if indices are sequential in the gpu transform buffer
			{
				if (first_index_of_chunk == -1) {
					transforms.push_back(m_registry.get<TransformComponent>(m_instances_to_update[i - 1]).GetMatrix());
					transforms.push_back(m_registry.get<TransformComponent>(m_instances_to_update[i]).GetMatrix());
					first_index_of_chunk = m_instances[m_instances_to_update[i - 1]];
				}
				else {
					transforms.push_back(m_registry.get<TransformComponent>(m_instances_to_update[i]).GetMatrix());
				}
			}
			else if (first_index_of_chunk != -1) // This is the end of the chunk, so update for this chunk
			{
				glNamedBufferSubData(m_transform_ssbo.GetHandle(), first_index_of_chunk * sizeof(glm::mat4), transforms.size() * sizeof(glm::mat4), &transforms[0]);
				transforms.clear();
				first_index_of_chunk = -1;

				// Update current transform (i) separately as it's not a part of the chunk
				glNamedBufferSubData(m_transform_ssbo.GetHandle(), m_instances[m_instances_to_update[i]] * sizeof(glm::mat4), sizeof(glm::mat4), &m_registry.get<TransformComponent>(m_instances_to_update[i]).GetMatrix()[0][0]);
			}
			else {
				// Ensure no transforms get skipped
				glNamedBufferSubData(m_transform_ssbo.GetHandle(), m_instances[m_instances_to_update[i]] * sizeof(glm::mat4), sizeof(glm::mat4), &m_registry.get<TransformComponent>(m_instances_to_update[i]).GetMatrix()[0][0]);
			}
		}

		// Last check, if the chunk persisted until the end of the array
		if (first_index_of_chunk != -1) {
			glNamedBufferSubData(m_transform_ssbo.GetHandle(), first_index_of_chunk * sizeof(glm::mat4), transforms.size() * sizeof(glm::mat4), &transforms[0]);
		}

		m_instances_to_update.clear();
	}

	void MeshInstanceGroup::ReallocateInstances() {
		m_transform_ssbo.data.resize(m_instances.size() * sizeof(glm::mat4) / sizeof(float));
		std::byte* p_byte = reinterpret_cast<std::byte*>(m_transform_ssbo.data.data());

		unsigned current_idx = 0;
		for (auto& [instance_handle, transform_idx] : m_instances) {
			transform_idx = current_idx++;
			ConvertToBytes(p_byte, m_registry.get<TransformComponent>(instance_handle).GetMatrix());
		}

		m_transform_ssbo.FillBuffer();
		m_used_transform_memory_end_idx = m_instances.size();
		m_transform_ssbo.data.clear();
		m_tombstone_count = 0;
	}


	void MeshInstanceGroup::AddInstance(SceneEntity* ptr) {
		std::array<std::byte, sizeof(glm::mat4)> transform_bytes;
		std::byte* p_byte = &transform_bytes[0];
		ConvertToBytes(p_byte, ptr->GetComponent<TransformComponent>()->GetMatrix());

		m_entities_to_instance.push_back(ptr->GetEnttHandle());
	}
}