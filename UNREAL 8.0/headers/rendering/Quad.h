#pragma once
#include <glm/glm.hpp>
class Quad {
public:
	void Load();
	void Draw();


private:

	glm::fvec3 vertices[6]{
	glm::fvec3(-1.0f, -1.0f, 0.0f),
	glm::fvec3(1.0f, -1.0f, 0.0f),
	glm::fvec3(-1.0f, 1.0f, 0.0f),

	glm::fvec3(-1.0f, 1.0f, 0.0f),
	glm::fvec3(1.0f, -1.0f, 0.0f),
	glm::fvec3(1.0f, 1.0f, 0.0f),
	};

	glm::fvec2 tex_coords[6] = {
		glm::fvec2(0.0f, 0.0f),
		glm::fvec2(1.0f, 0.0f),
		glm::fvec2(0.0f, 1.0f),
		glm::fvec2(0.0f, 1.0f),
		glm::fvec2(1.0f, 0.0f),
		glm::fvec2(1.0f, 1.0f),
	};
	unsigned int m_vao;
	unsigned int m_pos_buffer;
	unsigned int m_tex_buffer;
};
