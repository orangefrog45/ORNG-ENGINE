#include "shaders/LightingShader.h"
#include "GLErrorHandling.h"
#include <iostream>
#include <format>

void LightingShader::Init() {
	GLCall(unsigned int tprogramID = glCreateProgram());

	CompileShader(GL_VERTEX_SHADER, ParseShader("res/shaders/LightingVS.shader"), vert_shader_id);
	CompileShader(GL_FRAGMENT_SHADER, ParseShader("res/shaders/LightingFS.shader"), frag_shader_id);

	UseShader(vert_shader_id, tprogramID);
	UseShader(frag_shader_id, tprogramID);

	SetProgramID(tprogramID);

	GenUBOs();
	InitUniforms();
}


void LightingShader::GenUBOs() {
	GLCall(glGenBuffers(1, &m_matrix_UBO));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_matrix_UBO));
	GLCall(glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::fmat4), NULL, GL_STATIC_DRAW));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
	GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, UniformBindingPoints::PVMATRICES, m_matrix_UBO));

	GLCall(glGenBuffers(1, &m_point_light_UBO));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_point_light_UBO));
	GLCall(glBufferData(GL_UNIFORM_BUFFER, 56 * max_point_lights, NULL, GL_STATIC_DRAW));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
	GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, UniformBindingPoints::POINT_LIGHTS, m_point_light_UBO));

}

void LightingShader::SetMatrixUBOs(glm::fmat4& proj, glm::fmat4& view) {
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_matrix_UBO));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::fmat4), &proj[0][0]));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::fmat4), sizeof(glm::fmat4), &view[0][0]));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}

void LightingShader::SetAmbientLight(const BaseLight& light) {
	glm::fvec3 light_color = light.GetColor();
	glUniform3f(m_ambient_light_color_loc, light_color.x, light_color.y, light_color.z);
	glUniform1f(m_light_ambient_intensity_loc, light.GetAmbientIntensity());
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
	//if this crashes it is most likely due to an array in the fragment shader being too small to hold some data, switch to UBO'S soon
	GLCall(glLinkProgram(GetProgramID()));
	GLCall(glValidateProgram(GetProgramID()));
	GLCall(glUseProgram(GetProgramID()));

	GLCall(glDeleteShader(vert_shader_id));
	GLCall(glDeleteShader(frag_shader_id));
}

void LightingShader::InitUniforms() {
	ActivateProgram();
	m_sampler_location = GetUniform("gSampler");
	m_camera_view_pos_loc = GetUniform("view_pos");
	m_material_ambient_color_loc = GetUniform("g_material.ambient_color");
	m_material_specular_color_loc = GetUniform("g_material.specular_color");
	m_material_diffuse_color_loc = GetUniform("g_material.diffuse_color");
	m_sampler_specular_loc = GetUniform("specular_sampler");
	m_specular_sampler_active_loc = GetUniform("specular_sampler_active");
	m_num_point_light_loc = GetUniform("g_num_point_lights");
	m_num_spot_light_loc = GetUniform("g_num_spot_lights");
	m_light_ambient_intensity_loc = GetUniform("g_ambient_light.ambient_intensity");
	m_ambient_light_color_loc = GetUniform("g_ambient_light.color");


	for (unsigned int i = 0; i < max_point_lights; i++) {
		std::string ref;

		ref = std::format("g_point_lights[{}].base.color", i);
		m_point_light_locations[i].color = GetUniform(ref);

		ref = std::format("g_point_lights[{}].base.ambient_intensity", i);
		m_point_light_locations[i].ambient_intensity = GetUniform(ref);

		ref = std::format("g_point_lights[{}].base.diffuse_intensity", i);
		m_point_light_locations[i].diffuse_intensity = GetUniform(ref);

		ref = std::format("g_point_lights[{}].pos", i);
		m_point_light_locations[i].position = GetUniform(ref);

		ref = std::format("g_point_lights[{}].max_distance", i);
		m_point_light_locations[i].max_distance = GetUniform(ref);

		ref = std::format("g_point_lights[{}].atten.constant", i);
		m_point_light_locations[i].Atten.constant = GetUniform(ref);

		ref = std::format("g_point_lights[{}].atten.a_linear", i);
		m_point_light_locations[i].Atten.linear = GetUniform(ref);

		ref = std::format("g_point_lights[{}].atten.exp", i);
		m_point_light_locations[i].Atten.exp = GetUniform(ref);
	}

	for (unsigned int i = 0; i < max_spot_lights; i++) {
		std::string ref;

		ref = std::format("g_spot_lights[{}].base.base.color", i);
		m_spot_light_locations[i].base.color = GetUniform(ref);

		ref = std::format("g_spot_lights[{}].base.base.ambient_intensity", i);
		m_spot_light_locations[i].base.ambient_intensity = GetUniform(ref);

		ref = std::format("g_spot_lights[{}].base.base.diffuse_intensity", i);
		m_spot_light_locations[i].base.diffuse_intensity = GetUniform(ref);

		ref = std::format("g_spot_lights[{}].base.pos", i);
		m_spot_light_locations[i].base.position = GetUniform(ref);

		ref = std::format("g_spot_lights[{}].base.max_distance", i);
		m_spot_light_locations[i].base.max_distance = GetUniform(ref);

		ref = std::format("g_spot_lights[{}].base.atten.constant", i);
		m_spot_light_locations[i].base.Atten.constant = GetUniform(ref);

		ref = std::format("g_spot_lights[{}].base.atten.a_linear", i);
		m_spot_light_locations[i].base.Atten.linear = GetUniform(ref);

		ref = std::format("g_spot_lights[{}].base.atten.exp", i);
		m_spot_light_locations[i].base.Atten.exp = GetUniform(ref);

		ref = std::format("g_spot_lights[{}].dir", i);
		m_spot_light_locations[i].direction = GetUniform(ref);

		ref = std::format("g_spot_lights[{}].aperture", i);
		m_spot_light_locations[i].aperture = GetUniform(ref);
	}
}

void LightingShader::SetPointLights(std::vector< PointLight*>& p_lights) {

	for (unsigned int i = 0; i < p_lights.size(); i++) {

		glm::fvec3 light_color = p_lights[i]->GetColor();
		glUniform3f(m_point_light_locations[i].color, light_color.x, light_color.y, light_color.z);

		glUniform1f(m_point_light_locations[i].ambient_intensity, p_lights[i]->GetAmbientIntensity());

		glUniform1f(m_point_light_locations[i].diffuse_intensity, p_lights[i]->GetDiffuseIntensity());

		glm::fvec3 pos = p_lights[i]->GetWorldTransform().GetPosition();
		glUniform3f(m_point_light_locations[i].position, pos.x, pos.y, pos.z);

		glUniform1f(m_point_light_locations[i].max_distance, p_lights[i]->GetMaxDistance());

		glUniform1f(m_point_light_locations[i].Atten.constant, p_lights[i]->GetAttentuation().constant);

		glUniform1f(m_point_light_locations[i].Atten.linear, p_lights[i]->GetAttentuation().linear);

		glUniform1f(m_point_light_locations[i].Atten.exp, p_lights[i]->GetAttentuation().exp);
	}
	glUniform1i(m_num_point_light_loc, p_lights.size());
}

void LightingShader::SetSpotLights(std::vector<SpotLight*>& s_lights) {
	for (unsigned int i = 0; i < s_lights.size(); i++) {

		glm::fvec3 light_color = s_lights[i]->GetColor();
		glUniform3f(m_spot_light_locations[i].base.color, light_color.x, light_color.y, light_color.z);

		glUniform1f(m_spot_light_locations[i].base.ambient_intensity, s_lights[i]->GetAmbientIntensity());

		glUniform1f(m_spot_light_locations[i].base.diffuse_intensity, s_lights[i]->GetDiffuseIntensity());

		glm::fvec3 pos = s_lights[i]->GetWorldTransform().GetPosition();
		glUniform3f(m_spot_light_locations[i].base.position, pos.x, pos.y, pos.z);

		glUniform1f(m_spot_light_locations[i].base.max_distance, s_lights[i]->GetMaxDistance());

		glUniform1f(m_spot_light_locations[i].base.Atten.constant, s_lights[i]->GetAttentuation().constant);

		glUniform1f(m_spot_light_locations[i].base.Atten.linear, s_lights[i]->GetAttentuation().linear);

		glUniform1f(m_spot_light_locations[i].base.Atten.exp, s_lights[i]->GetAttentuation().exp);

		GLCall(glUniform1f(m_spot_light_locations[i].aperture, s_lights[i]->GetAperture()));

		glm::fvec3 light_dir = s_lights[i]->GetLightDirection();
		glUniform3f(m_spot_light_locations[i].direction, light_dir.x, light_dir.y, light_dir.z);
	}
	glUniform1i(m_num_spot_light_loc, s_lights.size());
}

const GLint& LightingShader::GetDiffuseSamplerLocation() {
	return m_sampler_location;
}

const GLint& LightingShader::GetSpecularSamplerLocation() {
	return m_sampler_specular_loc;
}
