#include "pch/pch.h"

#include "rendering/Quad.h"
#include "util/util.h"
namespace ORNG {

	void Quad::Load() {

		m_vao.vertex_data.positions = {
			glm::vec3(-1.0f, -1.0f, 0.f),
			 glm::vec3(1.0f, -1.0f, 0.f),
			glm::vec3(-1.0f, 1.0f, 0.f),

			glm::vec3(-1.0f, 1.0f, 0.f),
			 glm::vec3(1.0f, -1.0f, 0.f),
			 glm::vec3(1.0f, 1.0f, 0.f),
		};

		m_vao.vertex_data.tex_coords = {
			glm::vec2(0.0f, 0.0f),
			glm::vec2(1.0f, 0.0f),
			glm::vec2(0.0f, 1.0f),
			glm::vec2(0.0f, 1.0f),
			glm::vec2(1.0f, 0.0f),
			glm::vec2(1.0f, 1.0f),
		};

		m_vao.FillBuffers();

	}
}