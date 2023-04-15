#pragma once
#include <glew.h>
#include <vector>
#include <glm/glm.hpp>
class GridMesh
{
public:
	void Init();
	void Draw();
	void CheckBoundary(glm::fvec3 camera_pos);

private:
	float salt_grid_width = 400.0f;
	float salt_center_x;
	float salt_center_z;
	GLuint salt_m_VAO;
	GLuint salt_m_vertex_buffer;
	GLuint salt_m_world_mat_buffer;
	std::vector<float> salt_vertexArray = {0.0f, 0.0f, 0.0f,
										   salt_grid_width, 0.0f, 0.0f};

	std::vector<glm::fmat4> transform_array;
};