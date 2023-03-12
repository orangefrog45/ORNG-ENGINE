#pragma once
#include <glm/glm.hpp>
#include "WorldTransform.h"
class Quad {
public:
	void Load();
	void Draw();
	void SetScale(float x, float y) { m_world_transform.SetScale(x, y); };
	void SetPosition(float x, float y) { m_world_transform.SetPosition(x, y); }
	void SetRotation(float rot) { m_world_transform.SetRotation(rot); }
	const WorldTransform2D& GetTransform() const { return m_world_transform; }


private:
	WorldTransform2D m_world_transform;

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
	glm::fvec2 pos = glm::fvec2(0.0f, 0.0f);
	glm::fvec2 scale = glm::fvec2(1.0f, 1.0f);
	unsigned int m_vao;
	unsigned int m_pos_buffer;
	unsigned int m_tex_buffer;
};
