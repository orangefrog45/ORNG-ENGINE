#include <glew.h>
#include <ShaderLibrary.h>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <glfw/glfw3.h>
#include <vector>
#include <algorithm>
#include "GLErrorHandling.h"
#include "util/util.h"

void ShaderLibrary::Init() {
	grid_shader.Init();
	lighting_shader.Init();
	flat_color_shader.Init();
	depth_shader.Init();
	basic_sampler_shader.Init();
	reflection_shader.Init();
	cube_map_shadow_shader.Init();
	g_buffer_shader.Init();
	skybox_shader.Init();

	GLCall(glGenBuffers(1, &m_matrix_UBO));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_matrix_UBO));
	GLCall(glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::fmat4), NULL, GL_DYNAMIC_DRAW));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
	GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, RendererResources::UniformBindingPoints::PVMATRICES, m_matrix_UBO));

}

void ShaderLibrary::SetMatrixUBOs(glm::fmat4& proj, glm::fmat4& view) {
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_matrix_UBO));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::fmat4), &proj[0][0]));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::fmat4), sizeof(glm::fmat4), &view[0][0]));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}