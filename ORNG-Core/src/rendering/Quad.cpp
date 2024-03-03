#include "pch/pch.h"

#include "rendering/Quad.h"
#include "util/util.h"
namespace ORNG {
	void Quad::Load() {
		m_vao = std::make_unique<VAO>();
		m_vao->Init();
		auto* p_position_buffer = m_vao->AddBuffer<VertexBufferGL<float>>(0, GL_FLOAT, 2, GL_STATIC_DRAW);
		auto* p_tex_coord_buffer = m_vao->AddBuffer<VertexBufferGL<float>>(1, GL_FLOAT, 2, GL_STATIC_DRAW);

		p_position_buffer->data = {
			-1.0f, -1.0f,
			1.0f, -1.0f,
			-1.0f, 1.0f,

			-1.0f, 1.0f,
			1.0f, -1.0f,
			1.0f, 1.0f,
		};

		p_tex_coord_buffer->data = {
			0.0f, 0.0f,
			1.0f, 0.0f,
			0.0f, 1.0f,
			0.0f, 1.0f,
			1.0f, 0.0f,
			1.0f, 1.0f,
		};

		m_vao->FillBuffers();
	}

	void Quad::SetVertices(glm::vec2 min, glm::vec2 max) {
		ASSERT(m_vao);
		auto* p_position_buffer = m_vao->GetBuffer<VertexBufferGL<float>>(0);
		glm::vec2 tl = { min.x, max.y };
		glm::vec2 br = { max.x, min.y };

		p_position_buffer->data = {
			min.x, min.y,
			br.x, br.y,
			tl.x, tl.y,

			tl.x, tl.y,
			br.x, br.y,
			max.x, max.y
		};

		m_vao->FillBuffers();
	}
}