#pragma once
#include <glew.h>
#include <vector>
#include <glm/glm.hpp>
class GridMesh {
public:
	void Init();
	void Draw();
private:
	GLuint m_VAO;
	GLuint m_vertex_buffer;
	GLuint m_world_mat_buffer;
	std::vector<float> vertexArray = { 0.0f, 0.0f, 0.0f,
									   500.0f, 0.0f, 0.0f };

	std::vector<glm::fmat4> transform_array;
};