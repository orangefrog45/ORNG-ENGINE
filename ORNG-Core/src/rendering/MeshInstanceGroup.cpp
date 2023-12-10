#include "pch/pch.h"

#include "rendering/MeshAsset.h"
#include "scene/MeshInstanceGroup.h"
#include "components/ComponentSystems.h"
#include "components/TransformComponent.h"
#include "util/Log.h"
#include "scene/Scene.h"
#include "scene/SceneEntity.h"

namespace ORNG {
	void MeshInstanceGroup::RemoveInstance(SceneEntity* ptr) {
		unsigned instance_index = m_instances[ptr];
		m_instances.erase(ptr);

		// TODO: Batch deletes/adds too
		m_transform_ssbo.Erase(instance_index * sizeof(glm::mat4), sizeof(glm::mat4));

		auto it = std::ranges::find(m_instances_to_update, ptr);

		if (it != m_instances_to_update.end())
			m_instances_to_update.erase(std::ranges::find(m_instances_to_update, ptr));

		for (auto& [p_ent, index] : m_instances) {
			if (instance_index < index)
				index--;
		}
	}

	void MeshInstanceGroup::ClearMeshes() {
		while (!m_instances.empty()) {
			m_instances.erase(m_instances.begin());
		}
	}


	void MeshInstanceGroup::FlagInstanceTransformUpdate(SceneEntity* p_instance) {
		m_instances_to_update.push_back(p_instance);
	}


	void MeshInstanceGroup::ProcessUpdates() {
		if (m_instances_to_update.empty() || m_instances.empty())
			return;

		// Sort so neighbouring transforms can be found and updated all in one call to buffersubdata
		std::ranges::sort(m_instances_to_update, [this](const auto& p_ent, const auto& p_ent2) {return m_instances[p_ent] < m_instances[p_ent2]; });

		// Erase duplicate instances flagged for update
		m_instances_to_update.erase(std::unique(m_instances_to_update.begin(), m_instances_to_update.end()), m_instances_to_update.end());

		std::vector<glm::mat4> transforms;

		// Used for saving the first position of a "chunk" of transforms to be updated, so they can be more efficiently updated with fewer buffersubdata calls
		int first_index_of_chunk = -1;

		if (m_transform_ssbo.GetGPU_BufferSize() == 0)
			m_transform_ssbo.Resize(64);

		// Below loop can't cover index 0 (first check puts it out of range) so handle separately here
		transforms.push_back(m_instances_to_update[0]->GetComponent<TransformComponent>()->GetMatrix());
		glNamedBufferSubData(m_transform_ssbo.GetHandle(), m_instances.at(m_instances_to_update[0]) * sizeof(glm::mat4), transforms.size() * sizeof(glm::mat4), &transforms[0]);
		transforms.clear();

		for (int i = 1; i < m_instances_to_update.size(); i++) {
			if (m_instances.at(m_instances_to_update[i]) == m_instances.at(m_instances_to_update[i - 1]) + 1) // Check if indices are sequential in the gpu transform buffer
			{
				if (first_index_of_chunk == -1) {
					transforms.push_back(m_instances_to_update[i - 1]->GetComponent<TransformComponent>()->GetMatrix());
					transforms.push_back(m_instances_to_update[i]->GetComponent<TransformComponent>()->GetMatrix());
					first_index_of_chunk = m_instances.at(m_instances_to_update[i - 1]);
				}
				else {
					transforms.push_back(m_instances_to_update[i]->GetComponent<TransformComponent>()->GetMatrix());
				}
			}
			else if (first_index_of_chunk != -1) // This is the end of the chunk, so update for this chunk
			{
				glNamedBufferSubData(m_transform_ssbo.GetHandle(), first_index_of_chunk * sizeof(glm::mat4), transforms.size() * sizeof(glm::mat4), &transforms[0]);
				transforms.clear();
				transforms.reserve((m_instances_to_update.size() - i) * 0.1);
				first_index_of_chunk = -1;

				// Update current transform (i) separately as it's not a part of the chunk
				glNamedBufferSubData(m_transform_ssbo.GetHandle(), m_instances.at(m_instances_to_update[i]) * sizeof(glm::mat4), sizeof(glm::mat4), &m_instances_to_update[i]->GetComponent<TransformComponent>()->GetMatrix()[0][0]);
			}
			else {
				// Ensure no transforms get skipped
				glNamedBufferSubData(m_transform_ssbo.GetHandle(), m_instances.at(m_instances_to_update[i]) * sizeof(glm::mat4), sizeof(glm::mat4), &m_instances_to_update[i]->GetComponent<TransformComponent>()->GetMatrix()[0][0]);
			}
		}

		// Last check, if the chunk persisted until the end of the array
		if (first_index_of_chunk != -1) {
			glNamedBufferSubData(m_transform_ssbo.GetHandle(), first_index_of_chunk * sizeof(glm::mat4), transforms.size() * sizeof(glm::mat4), &transforms[0]);
		}

		m_instances_to_update.clear();
	}

	void MeshInstanceGroup::AddInstance(SceneEntity* ptr) {
		std::vector<std::byte> transform_bytes;
		transform_bytes.resize(sizeof(glm::mat4));
		std::byte* p_byte = &transform_bytes[0];
		PushMatrixIntoArrayBytes(ptr->GetComponent<TransformComponent>()->GetMatrix(), p_byte);

		// TODO: Batch add operations
		m_transform_ssbo.PushBack(&transform_bytes[0], sizeof(glm::mat4));

		m_instances[ptr] = m_instances.size();
	}
}