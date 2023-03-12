#include "shaders/LightingShader.h"
#include "GLErrorHandling.h"
#include <iostream>
#include <future>
#include <format>


void LightingShader::GenUBOs() {
	GLCall(glGenBuffers(1, &m_matrix_UBO));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_matrix_UBO));
	GLCall(glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::fmat4), NULL, GL_DYNAMIC_DRAW));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
	GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, UniformBindingPoints::PVMATRICES, m_matrix_UBO));

	GLCall(glGenBuffers(1, &m_point_light_UBO));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_point_light_UBO));
	GLCall(glBufferData(GL_UNIFORM_BUFFER, sizeof(float) * point_light_fs_num_float * max_point_lights, NULL, GL_DYNAMIC_DRAW));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
	GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, UniformBindingPoints::POINT_LIGHTS, m_point_light_UBO));

	GLCall(glGenBuffers(1, &m_spot_light_UBO));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_spot_light_UBO));
	GLCall(glBufferData(GL_UNIFORM_BUFFER, sizeof(float) * spot_light_fs_num_float * max_spot_lights, NULL, GL_DYNAMIC_DRAW));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
	GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, UniformBindingPoints::SPOT_LIGHTS, m_spot_light_UBO));
}

void LightingShader::Init() {
	GLCall(unsigned int tprogramID = glCreateProgram());

	CompileShader(GL_VERTEX_SHADER, ParseShader(paths[0]), m_vert_shader_id);
	CompileShader(GL_FRAGMENT_SHADER, ParseShader(paths[1]), m_frag_shader_id);

	UseShader(m_vert_shader_id, tprogramID);
	UseShader(m_frag_shader_id, tprogramID);

	SetProgramID(tprogramID);

	GenUBOs();
	InitUniforms();
}

void LightingShader::SetMatrixUBOs(glm::fmat4& proj, glm::fmat4& view) {
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_matrix_UBO));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::fmat4), &proj[0][0]));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::fmat4), sizeof(glm::fmat4), &view[0][0]));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}

void LightingShader::SetAmbientLight(const BaseLight& light) {
	glm::fvec3 light_color = light.GetColor();
	glUniform4f(m_ambient_light_color_loc, light_color.x, light_color.y, light_color.z, 1);
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
	glUniform1i(m_sampler_texture_col_location, unit);
}

void LightingShader::SetSpecularTextureUnit(unsigned int unit) {
	glUniform1i(m_sampler_specular_loc, unit);
}

void LightingShader::SetShadowMapTextureUnit(unsigned int unit) {
	glUniform1i(m_sampler_shadow_map_loc, unit);
}


void LightingShader::InitUniforms() {
	ActivateProgram();
	m_sampler_texture_col_location = GetUniform("gSampler");
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
	m_dir_light_color_loc = GetUniform("directional_light.color");
	m_dir_light_dir_loc = GetUniform("directional_light.direction");
	m_dir_light_diffuse_intensity_loc = GetUniform("directional_light.diffuse_intensity");
	m_light_space_mat_loc = GetUniform("dir_light_matrix");
	m_sampler_shadow_map_loc = GetUniform("shadow_map");

	SetDiffuseTextureUnit(TextureUnitIndexes::COLOR_TEXTURE_UNIT_INDEX);
	SetSpecularTextureUnit(TextureUnitIndexes::SPECULAR_TEXTURE_UNIT_INDEX);
	SetShadowMapTextureUnit(TextureUnitIndexes::SHADOW_MAP_TEXTURE_UNIT_INDEX);
}

void LightingShader::SetDirectionLight(const DirectionalLight& light) {
	auto color = light.GetColor();
	glUniform3f(m_dir_light_color_loc, color.x, color.y, color.z);
	auto dir = glm::normalize(light.GetLightDirection());
	glUniform3f(m_dir_light_dir_loc, dir.x, dir.y, dir.z);
	glUniform1f(m_dir_light_diffuse_intensity_loc, light.GetDiffuseIntensity());
	glUniform1f(m_dir_light_ambient_intensity_loc, light.GetAmbientIntensity());
}



void LightingShader::SetPointLights(std::vector< PointLight*>& p_lights) {

	glUniform1i(m_num_point_light_loc, p_lights.size());
	float light_array[max_point_lights * point_light_fs_num_float] = { 0 };

	for (unsigned int i = 0; i < p_lights.size() * point_light_fs_num_float; i += point_light_fs_num_float) {
		//0 - START COLOR
		auto color = p_lights[i / point_light_fs_num_float]->GetColor();
		light_array[i] = color.x;
		light_array[i + 1] = color.y;
		light_array[i + 2] = color.z;
		light_array[i + 3] = 0; //padding
		//16 - END COLOR - START POS
		auto pos = p_lights[i / point_light_fs_num_float]->GetWorldTransform().GetPosition();
		light_array[i + 4] = pos.x;
		light_array[i + 5] = pos.y;
		light_array[i + 6] = pos.z;
		light_array[i + 7] = 0; //padding
		//32 - END POS, START INTENSITY
		light_array[i + 8] = p_lights[i / point_light_fs_num_float]->GetAmbientIntensity();
		light_array[i + 9] = p_lights[i / point_light_fs_num_float]->GetDiffuseIntensity();
		//40 - END INTENSITY - START MAX_DISTANCE
		light_array[i + 10] = p_lights[i / point_light_fs_num_float]->GetMaxDistance();
		//44 - END MAX_DISTANCE - START ATTENUATION
		auto& atten = p_lights[i / point_light_fs_num_float]->GetAttentuation();
		light_array[i + 11] = atten.constant;
		light_array[i + 12] = atten.linear;
		light_array[i + 13] = atten.exp;
		//52 - END ATTENUATION
		light_array[i + 14] = 0; //padding
		light_array[i + 15] = 0; //padding
		//64 BYTES TOTAL
	}


	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_point_light_UBO));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(float) * point_light_fs_num_float * p_lights.size(), &light_array[0]));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));

}

void LightingShader::SetSpotLights(std::vector<SpotLight*>& s_lights) {
	glUniform1i(m_num_spot_light_loc, s_lights.size());
	float light_array[max_spot_lights * spot_light_fs_num_float] = { 0 };

	for (unsigned int i = 0; i < s_lights.size() * spot_light_fs_num_float; i += spot_light_fs_num_float) {
		//0 - START COLOR
		auto color = s_lights[i / spot_light_fs_num_float]->GetColor();
		light_array[i] = color.x;
		light_array[i + 1] = color.y;
		light_array[i + 2] = color.z;
		light_array[i + 3] = 0; //padding
		//16 - END COLOR - START POS
		auto pos = s_lights[i / spot_light_fs_num_float]->GetWorldTransform().GetPosition();
		light_array[i + 4] = pos.x;
		light_array[i + 5] = pos.y;
		light_array[i + 6] = pos.z;
		light_array[i + 7] = 0; //padding
		//32 - END POS, START DIR
		auto dir = s_lights[i / spot_light_fs_num_float]->GetLightDirection();
		light_array[i + 8] = dir.x;
		light_array[i + 9] = dir.y;
		light_array[i + 10] = dir.z;
		light_array[i + 11] = 0; // padding
		//48 - END DIR, START INTENSITY
		light_array[i + 12] = s_lights[i / spot_light_fs_num_float]->GetAmbientIntensity();
		light_array[i + 13] = s_lights[i / spot_light_fs_num_float]->GetDiffuseIntensity();
		//40 - END INTENSITY - START MAX_DISTANCE
		light_array[i + 14] = s_lights[i / spot_light_fs_num_float]->GetMaxDistance();
		//44 - END MAX_DISTANCE - START ATTENUATION
		auto& atten = s_lights[i / spot_light_fs_num_float]->GetAttentuation();
		light_array[i + 15] = atten.constant;
		light_array[i + 16] = atten.linear;
		light_array[i + 17] = atten.exp;
		//52 - END ATTENUATION - START APERTURE
		light_array[i + 18] = s_lights[i / spot_light_fs_num_float]->GetAperture();
		light_array[i + 19] = 0; //padding
	}

	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_spot_light_UBO));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(float) * spot_light_fs_num_float * s_lights.size(), &light_array[0]));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}

