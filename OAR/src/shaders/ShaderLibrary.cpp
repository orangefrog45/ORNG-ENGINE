#include "pch/pch.h"

#include "shaders/ShaderLibrary.h"
#include "util/util.h"
#include "rendering/Renderer.h"

void ShaderLibrary::Init() {
	lighting_shader.Init();
	lighting_shader.m_shader_id = 0;

	Shader& grid_shader = CreateShader("grid", { "res/shaders/GridVS.shader", "res/shaders/GridFS.shader" });
	grid_shader.AddUniform("camera_pos");

	Shader& depth_shader = CreateShader("depth", { "res/shaders/DepthVS.shader", "res/shaders/DepthFS.shader" });
	depth_shader.AddUniform("pv_matrix");

	GLCall(glGenBuffers(1, &m_matrix_UBO));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_matrix_UBO));
	GLCall(glBufferData(GL_UNIFORM_BUFFER, 3 * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW));
	GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, Renderer::UniformBindingPoints::PVMATRICES, m_matrix_UBO));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));

}

Shader& ShaderLibrary::CreateShader(const char* name, const std::array<const char*, 2>& paths) {
	m_shaders.try_emplace(name, name, CreateShaderID());
	m_shaders[name].paths = paths;
	m_shaders[name].Init();
	OAR_CORE_INFO("Shader '{0}' created", name);
	return m_shaders[name];
}

Shader& ShaderLibrary::GetShader(const char* name) {
	if (!m_shaders.contains(name)) {
		OAR_CORE_CRITICAL("No shader with name '{0}' exists", name);
		BREAKPOINT;
	}
	return m_shaders[name];
}

void ShaderLibrary::SetMatrixUBOs(glm::mat4& proj, glm::mat4& view) {
	glm::mat4 proj_view = proj * view;
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_matrix_UBO));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &proj[0][0]));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &view[0][0]));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, sizeof(glm::mat4), &proj_view[0][0]));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}