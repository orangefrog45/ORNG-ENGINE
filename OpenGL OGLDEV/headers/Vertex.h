#pragma once
#include <glm/glm.hpp>
struct Vertex {
	glm::fvec3 pos;
	glm::fvec2 tex;

	Vertex() {}
	Vertex(const glm::fvec3& pos_, const glm::fvec2& tex_);

};