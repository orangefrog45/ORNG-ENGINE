#include "pch/pch.h"
#include "rendering/VAO.h"
#include "util/Log.h"
#include "core/GLStateManager.h"
static constexpr unsigned int POSITION_LOCATION = 0;
static constexpr unsigned int TEX_COORD_LOCATION = 1;
static constexpr unsigned int NORMAL_LOCATION = 2;
static constexpr unsigned int TANGENT_LOCATION = 3;

namespace ORNG {

	void VAO::FillBuffers() {
		GL_StateManager::BindVAO(*this);

		if (!vertex_data.positions.empty()) {
			GL_StateManager::BindBuffer(GL_ARRAY_BUFFER, m_buffers[POS_VB]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data.positions[0]) * vertex_data.positions.size(), &vertex_data.positions[0], GL_STATIC_DRAW);
			glEnableVertexAttribArray(POSITION_LOCATION);
			glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);
		}

		if (!vertex_data.tex_coords.empty()) {
			GL_StateManager::BindBuffer(GL_ARRAY_BUFFER, m_buffers[TEXCOORD_VB]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data.tex_coords[0]) * vertex_data.tex_coords.size(), &vertex_data.tex_coords[0], GL_STATIC_DRAW);
			glEnableVertexAttribArray(TEX_COORD_LOCATION);
			glVertexAttribPointer(TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);
		}

		if (!vertex_data.normals.empty()) {
			GL_StateManager::BindBuffer(GL_ARRAY_BUFFER, m_buffers[NORMAL_VB]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data.normals[0]) * vertex_data.normals.size(), &vertex_data.normals[0], GL_STATIC_DRAW);
			glEnableVertexAttribArray(NORMAL_LOCATION);
			glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);
		}

		if (!vertex_data.indices.empty()) {
			GL_StateManager::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buffers[INDEX_BUFFER]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(vertex_data.indices[0]) * vertex_data.indices.size(), &vertex_data.indices[0], GL_STATIC_DRAW);
		}

		if (!vertex_data.tangents.empty()) {
			GL_StateManager::BindBuffer(GL_ARRAY_BUFFER, m_buffers[TANGENT_VB]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data.tangents[0]) * vertex_data.tangents.size(), &vertex_data.tangents[0], GL_STATIC_DRAW);
			glEnableVertexAttribArray(TANGENT_LOCATION);
			glVertexAttribPointer(TANGENT_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);
		}

		GL_StateManager::BindVAO(*this);

	}

	unsigned int VAO::GenTransformSSBO() {
		unsigned int ssbo_handle = GL_StateManager::GenBuffer();
		GL_StateManager::BindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_handle);

		m_transform_ssbo_handles.push_back(ssbo_handle);
		return ssbo_handle;
	}

	void VAO::FullUpdateTransformSSBO(unsigned int ssbo_handle, const std::vector<glm::mat4>* transforms, long long buffer_size) {
		if (std::find(m_transform_ssbo_handles.begin(), m_transform_ssbo_handles.end(), ssbo_handle) == m_transform_ssbo_handles.end()) {
			ORNG_CORE_CRITICAL("No transform ssbo with handle '{0}' located in VAO.");
			BREAKPOINT;
		}

		GL_StateManager::BindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_handle);


		if (buffer_size == -1 && transforms) // Size not specified, use automatically determined size
		{
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::mat4) * transforms->size(), transforms->data(), GL_DYNAMIC_DRAW);
		}
		else if (transforms == nullptr) // No transforms so just resize buffer
		{
			glBufferData(GL_SHADER_STORAGE_BUFFER, buffer_size, nullptr, GL_DYNAMIC_DRAW);
		}
		else // Resize buffer then update section with transforms
		{
			glBufferData(GL_SHADER_STORAGE_BUFFER, buffer_size, nullptr, GL_DYNAMIC_DRAW);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::mat4) * transforms->size(), transforms->data());
		}
	}

	void VAO::DeleteTransformSSBO(unsigned int ssbo_handle) {
		if (!VectorContains(m_transform_ssbo_handles, ssbo_handle)) {
			ORNG_CORE_ERROR("VAO does not contain SSBO with handle '{0}', failed to delete!", ssbo_handle);
		}
		else {
			glDeleteBuffers(1, &ssbo_handle);
		}

	}

	void VAO::SubUpdateTransformSSBO(unsigned int ssbo_handle, unsigned int index_offset, std::vector<glm::mat4>& transforms) {
		if (std::find(m_transform_ssbo_handles.begin(), m_transform_ssbo_handles.end(), ssbo_handle) == m_transform_ssbo_handles.end()) {
			ORNG_CORE_CRITICAL("No transform ssbo with handle '{0}' located in VAO.");
			BREAKPOINT;
		}

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_handle);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::mat4) * index_offset, sizeof(glm::mat4) * transforms.size(), transforms.data());

	}
}


