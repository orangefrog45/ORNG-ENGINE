#include <glm/glm.hpp>
#include "Quad.h"
#include "glew.h"
#include "glfw/glfw3.h"
#include "util/util.h"

void Quad::Load() {
	GLCall(glGenVertexArrays(1, &m_vao));
	GLCall(glGenBuffers(1, &m_pos_buffer));
	GLCall(glGenBuffers(1, &m_tex_buffer));
	GLCall(glBindVertexArray(m_vao));

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_pos_buffer));
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::fvec3) * 6, &vertices[0], GL_STATIC_DRAW));
	GLCall(glEnableVertexAttribArray(0));
	GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0));

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_tex_buffer));
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::fvec2) * 6, &tex_coords[0], GL_STATIC_DRAW));
	GLCall(glEnableVertexAttribArray(1));
	GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0));

	GLCall(glBindVertexArray(0));

}
