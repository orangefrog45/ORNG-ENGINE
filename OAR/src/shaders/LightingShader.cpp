#include "pch/pch.h"

#include "shaders/LightingShader.h"
#include "components/lights/PointLightComponent.h"
#include "components/lights/SpotLightComponent.h"
#include "rendering/Renderer.h"
#include "components/lights/DirectionalLight.h"


void LightingShader::GenUBOs() {
	AddUBO("pointlight_ubo", sizeof(float) * point_light_fs_num_float * Renderer::max_point_lights, GL_DYNAMIC_DRAW, Renderer::UniformBindingPoints::POINT_LIGHTS);
	AddUBO("spotlight_ubo", sizeof(float) * spot_light_fs_num_float * Renderer::max_spot_lights, GL_DYNAMIC_DRAW, Renderer::UniformBindingPoints::SPOT_LIGHTS);
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
	SetUniform("g_ambient_light.color", glm::vec4(light.color, 1));
	SetUniform("g_ambient_light.ambient_intensity", light.ambient_intensity);
}


void LightingShader::SetViewPos(const glm::vec3& pos) {
	SetUniform("view_pos", pos);
}

void LightingShader::SetMaterial(const Material& material) {

	if (material.diffuse_texture != nullptr) {
		Renderer::BindTexture(GL_TEXTURE_2D, material.diffuse_texture->GetTextureRef(), Renderer::TextureUnits::COLOR);
	}

	if (material.specular_texture == nullptr) {
		SetUniform("specular_sampler_active", 0);
	}
	else {
		SetUniform("specular_sampler_active", 1);
		Renderer::BindTexture(GL_TEXTURE_2D, material.specular_texture->GetTextureRef(), Renderer::TextureUnits::SPECULAR);
	}

	if (material.normal_map_texture == nullptr) {
		SetUniform("normal_sampler_active", 0);
	}
	else {
		SetUniform("normal_sampler_active", 1);
		Renderer::BindTexture(GL_TEXTURE_2D, material.normal_map_texture->GetTextureRef(), Renderer::TextureUnits::NORMAL_MAP);
	}

	SetUniform("g_material.ambient_color", material.ambient_color);
	SetUniform("g_material.specular_color", material.specular_color);
	SetUniform("g_material.diffuse_color", material.diffuse_color);
}


void LightingShader::InitUniforms() {
	AddUniforms({
			"terrain_mode",
			"view_pos",

			"g_material.ambient_color",
			"g_material.diffuse_color",
			"g_material.specular_color",

			"diffuse_sampler",
			"specular_sampler",
			"specular_sampler_active",
			"spot_depth_sampler",
			"dir_depth_sampler",
			"point_depth_sampler",
			"normal_map_sampler",
			"normal_sampler_active",
			"diffuse_array_sampler",
			"normal_array_sampler",
			"displacement_sampler",
			"displacement_sampler_active",

			"g_num_point_lights",
			"g_num_spot_lights",

			"g_ambient_light.ambient_intensity",
			"g_ambient_light.color",

			"directional_light.color",
			"directional_light.direction",
			"directional_light.diffuse_intensity",
			"dir_light_matrix"

		});

	SetUniform("diffuse_sampler", Renderer::TextureUnitIndexes::COLOR);
	SetUniform("specular_sampler", Renderer::TextureUnitIndexes::SPECULAR);
	SetUniform("spot_depth_sampler", Renderer::TextureUnitIndexes::SPOT_SHADOW_MAP);
	SetUniform("dir_depth_sampler", Renderer::TextureUnitIndexes::DIR_SHADOW_MAP);
	SetUniform("point_depth_sampler", Renderer::TextureUnitIndexes::POINT_SHADOW_MAP);
	SetUniform("normal_map_sampler", Renderer::TextureUnitIndexes::NORMAL_MAP);
	SetUniform("diffuse_array_sampler", Renderer::TextureUnitIndexes::DIFFUSE_ARRAY);
	SetUniform("normal_array_sampler", Renderer::TextureUnitIndexes::NORMAL_ARRAY);
	SetUniform("displacement_sampler", Renderer::TextureUnitIndexes::DISPLACEMENT);

}

void LightingShader::SetDirectionLight(const DirectionalLight& light) {
	SetUniform("directional_light.color", light.color);
	SetUniform("directional_light.direction", light.GetLightDirection());
	SetUniform("directional_light.diffuse_intensity", light.diffuse_intensity);
	SetUniform("dir_light_matrix", light.GetTransformMatrix());
}


void LightingShader::SetPointLights(std::vector< PointLightComponent*>& p_lights) {

	SetUniform("g_num_point_lights", static_cast<int>(p_lights.size()));
	constexpr int array_size = Renderer::max_point_lights * point_light_fs_num_float;
	std::array<float, array_size> light_array = { 0 };

	for (int i = 0; i < p_lights.size() * point_light_fs_num_float; i += point_light_fs_num_float) {
		auto& light = p_lights[i / point_light_fs_num_float];
		if (!light) continue;
		//0 - START COLOR
		auto color = light->color;
		light_array[i] = color.x;
		light_array[i + 1] = color.y;
		light_array[i + 2] = color.z;
		light_array[i + 3] = 0; //padding
		//16 - END COLOR - START POS
		auto pos = light->transform.GetPosition();
		light_array[i + 4] = pos.x;
		light_array[i + 5] = pos.y;
		light_array[i + 6] = pos.z;
		light_array[i + 7] = 0; //padding
		//32 - END POS, START INTENSITY
		light_array[i + 8] = light->ambient_intensity;
		light_array[i + 9] = light->diffuse_intensity;
		//40 - END INTENSITY - START MAX_DISTANCE
		light_array[i + 10] = light->max_distance;
		//44 - END MAX_DISTANCE - START ATTENUATION
		auto& atten = light->attenuation;
		light_array[i + 11] = atten.constant;
		light_array[i + 12] = atten.linear;
		light_array[i + 13] = atten.exp;
		//56 - END ATTENUATION
		light_array[i + 14] = 0; //padding
		light_array[i + 15] = 0; //padding
		//64 BYTES TOTAL
	}


	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_uniforms["pointlight_ubo"]));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(float) * light_array.size(), &light_array[0]));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));

}

void LightingShader::SetSpotLights(std::vector< SpotLightComponent*>& s_lights) {
	SetUniform("g_num_spot_lights", static_cast<int>(s_lights.size()));

	constexpr int array_size = Renderer::max_spot_lights * spot_light_fs_num_float;
	std::array<float, array_size> light_array = { 0 };

	for (int i = 0; i < s_lights.size() * spot_light_fs_num_float; i += spot_light_fs_num_float) {
		auto& light = s_lights[i / spot_light_fs_num_float];
		if (!light) continue;
		//0 - START COLOR
		auto color = light->color;
		light_array[i] = color.x;
		light_array[i + 1] = color.y;
		light_array[i + 2] = color.z;
		light_array[i + 3] = 0; //padding
		//16 - END COLOR - START POS
		auto pos = light->transform.GetPosition();
		light_array[i + 4] = pos.x;
		light_array[i + 5] = pos.y;
		light_array[i + 6] = pos.z;
		light_array[i + 7] = 0; //padding
		//32 - END POS, START DIR
		auto dir = light->GetLightDirection();
		light_array[i + 8] = dir.x;
		light_array[i + 9] = dir.y;
		light_array[i + 10] = dir.z;
		light_array[i + 11] = 0; // padding
		//48 - END DIR, START LIGHT TRANSFORM MAT
		glm::mat4 mat = light->GetLightSpaceTransformMatrix();
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
		light_array[i + 28] = light->ambient_intensity;
		light_array[i + 29] = light->diffuse_intensity;
		//40 - END INTENSITY - START MAX_DISTANCE
		light_array[i + 30] = light->max_distance;
		//44 - END MAX_DISTANCE - START ATTENUATION
		auto& atten = light->attenuation;
		light_array[i + 31] = atten.constant;
		light_array[i + 32] = atten.linear;
		light_array[i + 33] = atten.exp;
		//52 - END ATTENUATION - START APERTURE
		light_array[i + 34] = light->GetAperture();
		light_array[i + 35] = 0; //padding
	}

	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_uniforms["spotlight_ubo"]));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(float) * light_array.size(), &light_array[0]));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}

