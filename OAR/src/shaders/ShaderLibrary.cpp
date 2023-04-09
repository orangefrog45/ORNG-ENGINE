#include <ShaderLibrary.h>
#include "util/util.h"
#include "RendererResources.h"

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
	GLCall(glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
	GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, RendererResources::UniformBindingPoints::PVMATRICES, m_matrix_UBO));

}

void ShaderLibrary::SetMatrixUBOs(glm::mat4& proj, glm::mat4& view) {
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_matrix_UBO));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &proj[0][0]));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &view[0][0]));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}