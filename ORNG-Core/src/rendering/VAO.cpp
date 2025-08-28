#include "pch/pch.h"
#include "rendering/VAO.h"
#include "core/GLStateManager.h"
static constexpr unsigned int POSITION_LOCATION = 0;
static constexpr unsigned int TEX_COORD_LOCATION = 1;
static constexpr unsigned int NORMAL_LOCATION = 2;
static constexpr unsigned int TANGENT_LOCATION = 3;

namespace ORNG {
	BufferBase::BufferBase(GLenum t_buffer_type, bool is_mutable, GLbitfield flags) : buffer_type(t_buffer_type),
m_flags(flags), m_is_mutable(is_mutable) {  };

	BufferBase::~BufferBase() {
		if (GL_StateManager::GetPtr())
			GL_StateManager::DeleteBuffer(m_ogl_handle);
		else
			glDeleteBuffers(1, &m_ogl_handle);
	};

	VAO_Base::~VAO_Base() {
		glDeleteVertexArrays(1, &m_vao_handle);
	}

	void VAO::FillBuffers() {
		GL_StateManager::BindVAO(GetHandle());
		for (auto [index, p_buf] : m_buffers) {
			p_buf->FillBuffer();
		}


	}

	void VertexBufferBase::FillBuffer() {
		//glNamedBufferStorage(m_ogl_handle, GetSizeCPU(), GetDataPtr(), m_flags);
		GL_StateManager::BindBuffer(GL_ARRAY_BUFFER, m_ogl_handle);
		glBufferData(GL_ARRAY_BUFFER, GetSizeCPU(), GetDataPtr(), draw_type);

		glEnableVertexAttribArray(index);
		glVertexAttribPointer(index, comps_per_attribute, data_type, GL_FALSE, stride, nullptr);
	}

	void BufferBase::BufferSubData(size_t start, size_t size, const std::byte* p_data) {
		if (!m_is_mutable) {
			ORNG_CORE_ERROR("Calling BufferSubData on an immutable buffer isn't allowed");
		}

		glNamedBufferSubData(m_ogl_handle, start, size, p_data);
	}


	void BufferBase::FillBuffer() {
		GL_StateManager::BindBuffer(buffer_type, m_ogl_handle);

		if (m_is_mutable)
			glNamedBufferData(m_ogl_handle, GetSizeCPU(), GetDataPtr(), draw_type);
		else
			glNamedBufferStorage(m_ogl_handle, GetSizeCPU(), GetDataPtr(), m_flags);

	}

	void BufferBase::Init() {
		ASSERT(GL_StateManager::IsGlewIntialized());
		m_ogl_handle = GL_StateManager::GenBuffer();
		m_is_initalized = true;
		Resize(64);
	}

	size_t BufferBase::GetGPU_BufferSize() {
		GLint buffer_size;
		glGetNamedBufferParameteriv(m_ogl_handle, GL_BUFFER_SIZE, &buffer_size);
		return static_cast<size_t>(buffer_size);
	}

	void BufferBase::CopyDataFromBuffer(BufferBase& other, unsigned extra_allocated_size) {
		auto copy_size = other.GetGPU_BufferSize();
		GLuint buf = GL_StateManager::GenBuffer();

		if (m_is_mutable)
			glNamedBufferData(buf, copy_size + extra_allocated_size, nullptr, draw_type);
		else
			glNamedBufferStorage(buf, copy_size + extra_allocated_size, nullptr, m_flags);

		glCopyNamedBufferSubData(other.GetHandle(), buf, 0, 0, copy_size);

		m_ogl_handle = buf;

	}


	void BufferBase::Resize(size_t size_bytes) {
		GLuint buf = GL_StateManager::GenBuffer();
		size_bytes = glm::max(size_bytes, size_t{8});

		if (m_is_mutable)
			glNamedBufferData(buf, size_bytes, nullptr, draw_type);
		else
			glNamedBufferStorage(buf, size_bytes, nullptr, m_flags);

		GLint buffer_size;
		glGetNamedBufferParameteriv(m_ogl_handle, GL_BUFFER_SIZE, &buffer_size);

		// if shrinking don't want to copy everything, only up to size_bytes
		if (size_bytes < static_cast<size_t>(buffer_size)) {
			glCopyNamedBufferSubData(m_ogl_handle, buf, 0, 0, size_bytes);
		}
		else {
			glCopyNamedBufferSubData(m_ogl_handle, buf, 0, 0, buffer_size);
		}

		glDeleteBuffers(1, &m_ogl_handle);
		m_ogl_handle = buf;
	}

	void BufferBase::PushBack(std::byte* bytes, size_t size) {
		ORNG_TRACY_PROFILE;
		ASSERT(m_is_mutable);

		size_t old_size = GetGPU_BufferSize();
		Resize(old_size + size);
		glNamedBufferSubData(m_ogl_handle, old_size, size, bytes);
	}


	void BufferBase::Erase(size_t start, size_t erase_size) {
		GLint buffer_size;
		glGetNamedBufferParameteriv(m_ogl_handle, GL_BUFFER_SIZE, &buffer_size);

		GLuint buf = GL_StateManager::GenBuffer();
		if (m_is_mutable)
			glNamedBufferData(buf, buffer_size - static_cast<int>(erase_size), nullptr, draw_type);
		else
			glNamedBufferStorage(buf, glm::max(static_cast<int>(erase_size), 64), nullptr, m_flags);


		glCopyNamedBufferSubData(m_ogl_handle, buf, 0, 0, start);
		glCopyNamedBufferSubData(m_ogl_handle, buf, start + erase_size, start, static_cast<size_t>(buffer_size) - (start + erase_size));

		glDeleteBuffers(1, &m_ogl_handle);
		m_ogl_handle = buf;
	}



	MeshVAO::MeshVAO() {
		Init();
		glCreateBuffers(static_cast<GLsizei>(m_buffers.size()), &m_buffers[0]);
	};

	MeshVAO::~MeshVAO() {
		glDeleteBuffers(static_cast<GLsizei>(m_buffers.size()), &m_buffers[0]);
	};

	void MeshVAO::FillBuffers() {
		GL_StateManager::BindVAO(GetHandle());

		if (!vertex_data.positions.empty()) {
			GL_StateManager::BindBuffer(GL_ARRAY_BUFFER, m_buffers[POS_VB]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data.positions[0]) * vertex_data.positions.size(), &vertex_data.positions[0], GL_STATIC_DRAW);
			glEnableVertexAttribArray(POSITION_LOCATION);
			glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		}

		if (!vertex_data.tex_coords.empty()) {
			GL_StateManager::BindBuffer(GL_ARRAY_BUFFER, m_buffers[TEXCOORD_VB]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data.tex_coords[0]) * vertex_data.tex_coords.size(), &vertex_data.tex_coords[0], GL_STATIC_DRAW);
			glEnableVertexAttribArray(TEX_COORD_LOCATION);
			glVertexAttribPointer(TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
		}

		if (!vertex_data.normals.empty()) {
			GL_StateManager::BindBuffer(GL_ARRAY_BUFFER, m_buffers[NORMAL_VB]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data.normals[0]) * vertex_data.normals.size(), &vertex_data.normals[0], GL_STATIC_DRAW);
			glEnableVertexAttribArray(NORMAL_LOCATION);
			glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		}

		if (!vertex_data.indices.empty()) {
			GL_StateManager::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buffers[INDEX_BUFFER]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(vertex_data.indices[0]) * vertex_data.indices.size(), &vertex_data.indices[0], GL_STATIC_DRAW);
		}

		if (!vertex_data.tangents.empty()) {
			GL_StateManager::BindBuffer(GL_ARRAY_BUFFER, m_buffers[TANGENT_VB]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data.tangents[0]) * vertex_data.tangents.size(), &vertex_data.tangents[0], GL_STATIC_DRAW);
			glEnableVertexAttribArray(TANGENT_LOCATION);
			glVertexAttribPointer(TANGENT_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		}
	}
}
