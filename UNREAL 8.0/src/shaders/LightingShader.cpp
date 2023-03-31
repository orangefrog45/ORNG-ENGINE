#include <iostream>
#include <future>
#include <format>
#include "shaders/LightingShader.h"
#include "GLErrorHandling.h"
#include "RendererResources.h"


void LightingShader::GenUBOs() {
	GLCall(glGenBuffers(1, &m_point_light_UBO));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_point_light_UBO));
	GLCall(glBufferData(GL_UNIFORM_BUFFER, sizeof(float) * point_light_fs_num_float * RendererResources::max_point_lights, NULL, GL_DYNAMIC_DRAW));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
	GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, RendererResources::UniformBindingPoints::POINT_LIGHTS, m_point_light_UBO));

	GLCall(glGenBuffers(1, &m_spot_light_UBO));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_spot_light_UBO));
	GLCall(glBufferData(GL_UNIFORM_BUFFER, sizeof(float) * spot_light_fs_num_float * RendererResources::max_spot_lights, NULL, GL_DYNAMIC_DRAW));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
	GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, RendererResources::UniformBindingPoints::SPOT_LIGHTS, m_spot_light_UBO));
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


void LightingShader::SetAmbientLight(const BaseLight& light) {
	glm::fvec3 light_color = light.GetColor();
	glUniform4f(m_ambient_light_color_loc, light_color.x, light_color.y, light_color.z, 1);
	glUniform1f(m_light_ambient_intensity_loc, light.GetAmbientIntensity());
}


void LightingShader::SetViewPos(const glm::fvec3& pos) {
	glUniform3f(m_camera_view_pos_loc, pos.x, pos.y, pos.z);
}

void LightingShader::SetMaterial(const Material& material) {

	if (material.diffuse_texture == nullptr) {
		RendererResources::BindTexture(GL_TEXTURE_2D, RendererResources::GetMissingTexture()->GetTextureRef(), RendererResources::TextureUnits::COLOR);
	}
	else {
		RendererResources::BindTexture(GL_TEXTURE_2D, material.diffuse_texture->GetTextureRef(), RendererResources::TextureUnits::COLOR);
	}

	if (material.specular_texture == nullptr) {
		glUniform1i(m_specular_sampler_active_loc, 0);
	}
	else {
		glUniform1i(m_specular_sampler_active_loc, 1);
		RendererResources::BindTexture(GL_TEXTURE_2D, material.specular_texture->GetTextureRef(), RendererResources::TextureUnits::SPECULAR);
	}

	if (material.normal_map_texture == nullptr) {
		glUniform1i(m_normal_sampler_active_loc, 0);
	}
	else {
		glUniform1i(m_normal_sampler_active_loc, 1);
		RendererResources::BindTexture(GL_TEXTURE_2D, material.normal_map_texture->GetTextureRef(), RendererResources::TextureUnits::NORMAL_MAP);
	}

	glUniform3f(m_material_specular_color_loc, material.specular_color.r, material.specular_color.g, material.specular_color.b);
	glUniform3f(m_material_ambient_color_loc, material.ambient_color.r, material.ambient_color.g, material.ambient_color.b);
	glUniform3f(m_material_diffuse_color_loc, material.diffuse_color.r, material.diffuse_color.g, material.diffuse_color.b);
}

inline void LightingShader::SetDiffuseTextureUnit(unsigned int unit) {
	glUniform1i(m_sampler_texture_col_location, unit);
}

inline void LightingShader::SetSpecularTextureUnit(unsigned int unit) {
	glUniform1i(m_sampler_specular_loc, unit);
}

inline void LightingShader::SetDirDepthMapTexUnit(unsigned int unit) {
	glUniform1i(m_sampler_dir_depth_loc, unit);
}


inline void LightingShader::SetSpotDepthMapTexUnit(unsigned int unit) {
	glUniform1i(m_sampler_spot_depth_loc, unit);
}

inline void LightingShader::SetPointDepthMapTexUnit(unsigned int unit) {
	glUniform1i(m_sampler_point_depth_loc, unit);
}


void LightingShader::InitUniforms() {
	ActivateProgram();
	m_camera_view_pos_loc = GetUniform("view_pos");
	/* Material */
	m_material_ambient_color_loc = GetUniform("g_material.ambient_color");
	m_material_specular_color_loc = GetUniform("g_material.specular_color");
	m_material_diffuse_color_loc = GetUniform("g_material.diffuse_color");
	/* Samplers */
	m_sampler_texture_col_location = GetUniform("gSampler");
	m_sampler_specular_loc = GetUniform("specular_sampler");
	m_specular_sampler_active_loc = GetUniform("specular_sampler_active");
	m_sampler_spot_depth_loc = GetUniform("spot_depth_map");
	m_sampler_point_depth_loc = GetUniform("point_depth_map");
	m_sampler_normals_loc = GetUniform("normal_map");
	m_normal_sampler_active_loc = GetUniform("normal_map_active");

	m_num_point_light_loc = GetUniform("g_num_point_lights");
	m_num_spot_light_loc = GetUniform("g_num_spot_lights");

	m_light_ambient_intensity_loc = GetUniform("g_ambient_light.ambient_intensity");
	m_ambient_light_color_loc = GetUniform("g_ambient_light.color");

	m_dir_light_color_loc = GetUniform("directional_light.color");
	m_dir_light_dir_loc = GetUniform("directional_light.direction");
	m_dir_light_diffuse_intensity_loc = GetUniform("directional_light.diffuse_intensity");
	m_light_space_mat_loc = GetUniform("dir_light_matrix");

	SetDiffuseTextureUnit(RendererResources::TextureUnitIndexes::COLOR);
	SetSpecularTextureUnit(RendererResources::TextureUnitIndexes::SPECULAR);
	SetDirDepthMapTexUnit(RendererResources::TextureUnitIndexes::DIR_SHADOW_MAP);
	SetSpotDepthMapTexUnit(RendererResources::TextureUnitIndexes::SPOT_SHADOW_MAP);
	SetPointDepthMapTexUnit(RendererResources::TextureUnitIndexes::POINT_SHADOW_MAP);
	glUniform1i(m_sampler_normals_loc, RendererResources::TextureUnitIndexes::NORMAL_MAP);

}

void LightingShader::SetDirectionLight(const DirectionalLightComponent& light) {
	auto color = light.GetColor();
	glUniform3f(m_dir_light_color_loc, color.x, color.y, color.z);
	auto dir = glm::normalize(light.GetLightDirection());
	glUniform3f(m_dir_light_dir_loc, dir.x, dir.y, dir.z);
	glUniform1f(m_dir_light_diffuse_intensity_loc, light.GetDiffuseIntensity());
	glUniform1f(m_dir_light_ambient_intensity_loc, light.GetAmbientIntensity());
}



void LightingShader::SetPointLights(std::vector< PointLightComponent*>& p_lights) {

	glUniform1i(m_num_point_light_loc, p_lights.size());
	float light_array[RendererResources::max_point_lights * point_light_fs_num_float] = { 0 };

	for (int i = 0; i < p_lights.size() * point_light_fs_num_float; i += point_light_fs_num_float) {
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
		//56 - END ATTENUATION
		light_array[i + 14] = 0; //padding
		light_array[i + 15] = 0; //padding
		//64 BYTES TOTAL
	}


	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_point_light_UBO));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(float) * point_light_fs_num_float * p_lights.size(), &light_array[0]));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));

}

void LightingShader::SetSpotLights(std::vector<SpotLightComponent*>& s_lights) {
	glUniform1i(m_num_spot_light_loc, s_lights.size());
	float light_array[RendererResources::max_spot_lights * spot_light_fs_num_float] = { 0 };

	for (int i = 0; i < s_lights.size() * spot_light_fs_num_float; i += spot_light_fs_num_float) {
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
		//48 - END DIR, START LIGHT TRANSFORM MAT
		glm::fmat4 mat = s_lights[i / spot_light_fs_num_float]->GetTransformMatrix();
		light_array[i + 12] = mat[0][0];
		light_array[i + 13] = mat[0][1];
		light_array[i + 14] = mat[0][2];
		light_array[i + 15] = mat[0][3];
		light_array[i + 16] = mat[1][0];
		light_array[i + 17] = mat[1][1];
		light_array[i + 18] = mat[1][2];
		light_array[i + 19] = mat[1][3];
		light_array[i + 20] = mat[2][0];
		light_array[i + 21] = mat[2][1];
		light_array[i + 22] = mat[2][2];
		light_array[i + 23] = mat[2][3];
		light_array[i + 24] = mat[3][0];
		light_array[i + 25] = mat[3][1];
		light_array[i + 26] = mat[3][2];
		light_array[i + 27] = mat[3][3];
		//112   - END LIGHT TRANSFORM MAT, START INTENSITY
		light_array[i + 28] = s_lights[i / spot_light_fs_num_float]->GetAmbientIntensity();
		light_array[i + 29] = s_lights[i / spot_light_fs_num_float]->GetDiffuseIntensity();
		//40 - END INTENSITY - START MAX_DISTANCE
		light_array[i + 30] = s_lights[i / spot_light_fs_num_float]->GetMaxDistance();
		//44 - END MAX_DISTANCE - START ATTENUATION
		auto& atten = s_lights[i / spot_light_fs_num_float]->GetAttentuation();
		light_array[i + 31] = atten.constant;
		light_array[i + 32] = atten.linear;
		light_array[i + 33] = atten.exp;
		//52 - END ATTENUATION - START APERTURE
		light_array[i + 34] = s_lights[i / spot_light_fs_num_float]->GetAperture();
		light_array[i + 35] = 0; //padding
	}

	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_spot_light_UBO));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(float) * spot_light_fs_num_float * s_lights.size(), &light_array[0]));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}

