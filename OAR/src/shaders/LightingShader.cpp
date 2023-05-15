#include "pch/pch.h"

#include "shaders/LightingShader.h"
#include "components/lights/PointLightComponent.h"
#include "components/lights/SpotLightComponent.h"
#include "rendering/Renderer.h"
#include "components/lights/DirectionalLight.h"
#include "core/GLStateManager.h"

namespace ORNG {


	void LightingShader::SetAmbientLight(const BaseLight& light) {
		SetUniform("u_ambient_light.color", glm::vec4(light.color, 1));
		SetUniform("u_ambient_light.ambient_intensity", light.ambient_intensity);
	}


	void LightingShader::InitUniforms() {

		AddUniforms({
				"u_terrain_mode",
				"u_view_pos",

				"u_material.ambient_color",
				"u_material.diffuse_color",
				"u_material.specular_color",

				"spot_depth_sampler",
				"dir_depth_sampler",
				"point_depth_sampler",
				"albedo_sampler",
				"world_position_sampler",
				"normal_sampler",
				"shader_material_id_sampler",

				"u_num_point_lights",
				"u_num_spot_lights",

				"u_ambient_light.ambient_intensity",
				"u_ambient_light.color",

				"u_dir_light_matrices[0]",
				"u_dir_light_matrices[1]",
				"u_dir_light_matrices[2]",
				"u_directional_light.color",
				"u_directional_light.direction",
				"u_directional_light.diffuse_intensity",


			});

		SetUniform("spot_depth_sampler", GL_StateManager::TextureUnitIndexes::SPOT_SHADOW_MAP);
		SetUniform("albedo_sampler", GL_StateManager::TextureUnitIndexes::COLOR);
		SetUniform("world_position_sampler", GL_StateManager::TextureUnitIndexes::WORLD_POSITIONS);
		SetUniform("normal_sampler", GL_StateManager::TextureUnitIndexes::NORMAL_MAP);
		SetUniform("dir_depth_sampler", GL_StateManager::TextureUnitIndexes::DIR_SHADOW_MAP);
		SetUniform("point_depth_sampler", GL_StateManager::TextureUnitIndexes::POINT_SHADOW_MAP);
		SetUniform("shader_material_id_sampler", GL_StateManager::TextureUnitIndexes::SHADER_MATERIAL_IDS);
	}

	void LightingShader::SetDirectionLight(const DirectionalLight& light) {
		SetUniform("u_directional_light.color", light.color);
		SetUniform("u_directional_light.direction", light.GetLightDirection());
		SetUniform("u_directional_light.diffuse_intensity", light.diffuse_intensity);
	}


	void LightingShader::SetPointLights(std::vector< PointLightComponent*>& p_lights) {

		std::vector<float> light_array;

		std::vector<PointLightComponent*> active_lights;

		active_lights.reserve(p_lights.size());

		for (auto* light : p_lights) {
			if (light)
				active_lights.push_back(light);
		}

		light_array.resize(active_lights.size() * point_light_fs_num_float);

		for (int i = 0; i < active_lights.size() * point_light_fs_num_float; i += point_light_fs_num_float) {

			auto& light = active_lights[i / point_light_fs_num_float];

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

		GL_StateManager::BindBuffer(GL_SHADER_STORAGE_BUFFER, Renderer::GetShaderLibrary().GetPointlightSSBOHandle());
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * light_array.size(), &light_array[0], GL_DYNAMIC_DRAW);
	}

	void LightingShader::SetSpotLights(std::vector< SpotLightComponent*>& s_lights) {

		std::vector<float> light_array;

		int num_lights = 0;
		for (const auto* light : s_lights) {
			if (light)
				num_lights++;
		}

		light_array.resize(num_lights * spot_light_fs_num_float);


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

		GL_StateManager::BindBuffer(GL_SHADER_STORAGE_BUFFER, Renderer::GetShaderLibrary().GetSpotlightSSBOHandle());
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * light_array.size(), &light_array[0], GL_DYNAMIC_DRAW);
	}

}