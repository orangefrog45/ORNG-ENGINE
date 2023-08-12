#include "pch/pch.h"

#include "rendering/Quad.h"
#include "util/util.h"
namespace ORNG {

	void Quad::Load() {

		m_vao.vertex_data.positions = {
			-1.0f, -1.0f, 0.0f,
			1.0f, -1.0f, 0.0f,
			-1.0f, 1.0f, 0.0f,

			-1.0f, 1.0f, 0.0f,
			1.0f, -1.0f, 0.0f,
			1.0f, 1.0f, 0.0f,
		};

		m_vao.vertex_data.tex_coords = {
			0.0f, 0.0f,
			1.0f, 0.0f,
			0.0f, 1.0f,
			0.0f, 1.0f,
			1.0f, 0.0f,
			1.0f, 1.0f,
		};

		m_vao.FillBuffers();

	}
}