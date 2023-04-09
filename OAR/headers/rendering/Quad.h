#pragma once
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include "WorldTransform.h"
class Quad {
public:
	friend class Renderer;
	void Load();
	inline void SetScale(float x, float y) { m_world_transform.SetScale(x, y); };
	inline void SetPosition(float x, float y) { m_world_transform.SetPosition(x, y); }
	inline void SetRotation(float rot) { m_world_transform.SetRotation(rot); }
	const WorldTransform2D& GetTransform() const { return m_world_transform; }


private:
	WorldTransform2D m_world_transform;

	glm::vec3 vertices[6]{
	glm::vec3(-1.0f, -1.0f, 0.0f),
	glm::vec3(1.0f, -1.0f, 0.0f),
	glm::vec3(-1.0f, 1.0f, 0.0f),

	glm::vec3(-1.0f, 1.0f, 0.0f),
	glm::vec3(1.0f, -1.0f, 0.0f),
	glm::vec3(1.0f, 1.0f, 0.0f),
	};

	glm::vec2 tex_coords[6] = {
		glm::vec2(0.0f, 0.0f),
		glm::vec2(1.0f, 0.0f),
		glm::vec2(0.0f, 1.0f),
		glm::vec2(0.0f, 1.0f),
		glm::vec2(1.0f, 0.0f),
		glm::vec2(1.0f, 1.0f),
	};
	glm::vec2 pos = glm::vec2(0.0f, 0.0f);
	glm::vec2 scale = glm::vec2(1.0f, 1.0f);
	unsigned int m_vao;
	unsigned int m_pos_buffer;
	unsigned int m_tex_buffer;
};
