#pragma once
class GridMesh {
public:
	void Init();
	void Draw();
	void CheckBoundary(glm::vec3 camera_pos);
private:
	float grid_width = 400.0f;
	float center_x;
	float center_z;
	GLuint m_VAO;
	GLuint m_vertex_buffer;
	GLuint m_world_mat_buffer;
	std::vector<float> vertexArray = { 0.0f, 0.0f, 0.0f,
									   grid_width, 0.0f, 0.0f };

	std::vector<glm::mat4> transform_array;
};