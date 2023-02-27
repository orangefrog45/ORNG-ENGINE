#include "shaders/LightingShader.h"
#include "GLErrorHandling.h"
#include <iostream>

void LightingShader::Init() {
	GLCall(unsigned int tprogramID = glCreateProgram());

	CompileShader(GL_VERTEX_SHADER, ParseShader("res/shaders/LightingVS.shader"), vert_shader_id);
	CompileShader(GL_FRAGMENT_SHADER, ParseShader("res/shaders/LightingFS.shader"), frag_shader_id);

	UseShader(vert_shader_id, tprogramID);
	UseShader(frag_shader_id, tprogramID);

	SetProgramID(tprogramID);

	InitUniforms();
}

void LightingShader::SetCamera(const glm::fmat4& cam) {
	glUniformMatrix4fv(GetCameraLocation(), 1, GL_TRUE, &cam[0][0]);
}

void LightingShader::SetProjection(const glm::fmat4& proj) {
	glUniformMatrix4fv(GetProjectionLocation(), 1, GL_TRUE, &proj[0][0]);
}

void LightingShader::SetAmbientLight(const BaseLight& light) {
	glUniform3f(m_ambient_light_color_loc, light.color.x, light.color.y, light.color.z);
	glUniform1f(m_light_ambient_intensity_loc, light.ambient_intensity);
}

void LightingShader::SetPointLight(PointLight& light) {
	glUniform3f(m_point_light_color_loc, light.color.x, light.color.y, light.color.z);
	glm::fvec3 pos = light.transform.GetPosition();

	glUniform3f(m_point_light_position_loc, pos.x, pos.y, pos.z);
}

void LightingShader::SetViewPos(const glm::fvec3& pos) {
	glUniform3f(m_camera_view_pos_loc, pos.x, pos.y, pos.z);
}

void LightingShader::SetMaterial(const Material& material) {
	glUniform1i(m_specular_sampler_active_loc, material.specular_texture == nullptr ? 0 : 1);
	glUniform3f(m_material_specular_color_loc, material.specular_color.r, material.specular_color.g, material.specular_color.b);
	glUniform3f(m_material_ambient_color_loc, material.ambient_color.r, material.ambient_color.g, material.ambient_color.b);
	glUniform3f(m_material_diffuse_color_loc, material.diffuse_color.r, material.diffuse_color.g, material.diffuse_color.b);
}

void LightingShader::SetDiffuseTextureUnit(unsigned int unit) {
	glUniform1i(GetDiffuseSamplerLocation(), unit);
}

void LightingShader::SetSpecularTextureUnit(unsigned int unit) {
	glUniform1i(GetSpecularSamplerLocation(), unit);
}

void LightingShader::ActivateProgram() {
	GLCall(glLinkProgram(GetProgramID()));
	GLCall(glValidateProgram(GetProgramID()));
	GLCall(glUseProgram(GetProgramID()));

	GLCall(glDeleteShader(vert_shader_id));
	GLCall(glDeleteShader(frag_shader_id));


}

void LightingShader::InitUniforms() {
	ActivateProgram();
	m_projection_location = GetUniform("projection");
	m_camera_location = GetUniform("camera");
	m_sampler_location = GetUniform("gSampler");
	m_light_ambient_intensity_loc = GetUniform("g_light.ambient_intensity");
	m_ambient_light_color_loc = GetUniform("g_light.color");
	m_point_light_color_loc = GetUniform("g_point_light.color");
	m_point_light_position_loc = GetUniform("g_point_light.pos");
	m_camera_view_pos_loc = GetUniform("view_pos");
	m_material_ambient_color_loc = GetUniform("g_material.ambient_color");
	m_material_specular_color_loc = GetUniform("g_material.specular_color");
	m_material_diffuse_color_loc = GetUniform("g_material.diffuse_color");
	m_sampler_specular_loc = GetUniform("specular_sampler");
	m_specular_sampler_active_loc = GetUniform("specular_sampler_active");

}

const GLint& LightingShader::GetProjectionLocation() {
	return m_projection_location;
}


const GLint& LightingShader::GetCameraLocation() {
	return m_camera_location;
}

const GLint& LightingShader::GetDiffuseSamplerLocation() {
	return m_sampler_location;
}

const GLint& LightingShader::GetSpecularSamplerLocation() {
	return m_sampler_specular_loc;
}
