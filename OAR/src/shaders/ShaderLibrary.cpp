#include "pch/pch.h"

#include "shaders/ShaderLibrary.h"
#include "util/util.h"
#include "rendering/Renderer.h"
#include "core/GLStateManager.h"


namespace ORNG {

	void ShaderLibrary::Init() {

		m_pointlight_ssbo = GL_StateManager::GenBuffer();
		GL_StateManager::BindSSBO(m_pointlight_ssbo, GL_StateManager::SSBO_BindingPoints::POINT_LIGHTS);

		m_spotlight_ssbo = GL_StateManager::GenBuffer();
		GL_StateManager::BindSSBO(m_spotlight_ssbo, GL_StateManager::SSBO_BindingPoints::SPOT_LIGHTS);

		m_matrix_ubo = GL_StateManager::GenBuffer();
		GL_StateManager::BindBuffer(GL_UNIFORM_BUFFER, m_matrix_ubo);
		glBufferData(GL_UNIFORM_BUFFER, 3 * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::PVMATRICES, m_matrix_ubo);

		m_material_ubo = GL_StateManager::GenBuffer();
		GL_StateManager::BindBuffer(GL_UNIFORM_BUFFER, m_material_ubo);
		glBufferData(GL_UNIFORM_BUFFER, 128 * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::MATERIALS, m_material_ubo);

		m_global_lighting_ubo = GL_StateManager::GenBuffer();
		GL_StateManager::BindBuffer(GL_UNIFORM_BUFFER, m_global_lighting_ubo);
		glBufferData(GL_UNIFORM_BUFFER, 22 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::GLOBAL_LIGHTING, m_global_lighting_ubo);

		m_common_ubo = GL_StateManager::GenBuffer();
		GL_StateManager::BindBuffer(GL_UNIFORM_BUFFER, m_common_ubo);
		glBufferData(GL_UNIFORM_BUFFER, m_common_ubo_size, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::GLOBALS, m_common_ubo);

	}

	void ShaderLibrary::SetCommonUBO(glm::vec3 camera_pos, glm::vec3 camera_target) {
		std::array<float, m_common_ubo_size / sizeof(float)> data;

		data[0] = camera_pos.x;
		data[1] = camera_pos.y;
		data[2] = camera_pos.z;
		data[3] = 0.f; //padding
		data[4] = camera_target.x;
		data[5] = camera_target.y;
		data[6] = camera_target.z;
		data[7] = 0.f; //padding

		GL_StateManager::BindBuffer(GL_UNIFORM_BUFFER, m_common_ubo);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, m_common_ubo_size, &data[0]);
	}

	void ShaderLibrary::SetGlobalLighting(const DirectionalLight& dir_light, const BaseLight& ambient_light) {
		GL_StateManager::BindBuffer(GL_UNIFORM_BUFFER, m_global_lighting_ubo);

		constexpr int num_floats_in_buffer = 22;
		std::array<float, num_floats_in_buffer> light_data = { 0 };

		glm::vec3 light_dir = dir_light.GetLightDirection();
		light_data[0] = light_dir.x;
		light_data[1] = light_dir.y;
		light_data[2] = light_dir.z;
		light_data[3] = 0; //padding
		light_data[4] = dir_light.color.x;
		light_data[5] = dir_light.color.y;
		light_data[6] = dir_light.color.z;
		light_data[7] = 0; //padding
		light_data[8] = dir_light.ambient_intensity;
		light_data[9] = dir_light.diffuse_intensity;
		light_data[10] = 0; //padding
		light_data[11] = 0; //padding
		light_data[12] = dir_light.cascade_ranges[0];
		light_data[13] = dir_light.cascade_ranges[1];
		light_data[14] = dir_light.cascade_ranges[2];
		light_data[15] = 0; //padding
		light_data[16] = ambient_light.color.x;
		light_data[17] = ambient_light.color.y;
		light_data[18] = ambient_light.color.z;
		light_data[19] = 0;
		light_data[20] = ambient_light.ambient_intensity;
		light_data[21] = ambient_light.diffuse_intensity;

		glBufferSubData(GL_UNIFORM_BUFFER, 0, num_floats_in_buffer * sizeof(float), &light_data[0]);
	};

	void ShaderLibrary::UpdateMaterialUBO(const std::vector<Material*>& materials) { //SHOULDNT BE A UBO

		constexpr int material_size_floats = 8;
		std::array<float, Renderer::max_materials* material_size_floats> material_arr = { 0 };
		unsigned int index = 0;

		for (const auto* material : materials) {
			material_arr[index++] = material->base_color.x;
			material_arr[index++] = material->base_color.y;
			material_arr[index++] = material->base_color.z;
			material_arr[index++] = material->metallic;

			material_arr[index++] = material->roughness;
			material_arr[index++] = material->ao;
			material_arr[index++] = 0; //padding
			material_arr[index++] = 0; //padding
		}

		glBindBuffer(GL_UNIFORM_BUFFER, m_material_ubo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(float) * material_arr.size(), &material_arr, GL_DYNAMIC_DRAW);

	}
	Shader& ShaderLibrary::CreateShader(const char* name) {
		m_shaders.try_emplace(name, name, CreateShaderID());
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
		glBindBuffer(GL_UNIFORM_BUFFER, m_matrix_ubo);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &proj[0][0]);
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &view[0][0]);
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, sizeof(glm::mat4), &proj_view[0][0]);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void ShaderLibrary::SetPointLights(const std::vector<PointLightComponent*>& p_lights) {
		constexpr unsigned int point_light_fs_num_float = 16; // amount of floats in pointlight struct in shaders

		std::vector<float> light_array;

		light_array.resize(p_lights.size() * point_light_fs_num_float);

		for (int i = 0; i < p_lights.size() * point_light_fs_num_float; i += point_light_fs_num_float) {

			auto& light = p_lights[i / point_light_fs_num_float];

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

		GL_StateManager::BindBuffer(GL_SHADER_STORAGE_BUFFER, m_pointlight_ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * light_array.size(), &light_array[0], GL_DYNAMIC_DRAW);
	}

	void ShaderLibrary::SetSpotLights(const std::vector< SpotLightComponent*>& s_lights) {

		constexpr unsigned int spot_light_fs_num_float = 36; // amount of floats in spotlight struct in shaders
		std::vector<float> light_array;

		light_array.resize(s_lights.size() * spot_light_fs_num_float);


		for (int i = 0; i < s_lights.size() * spot_light_fs_num_float; i += spot_light_fs_num_float) {

			auto& light = s_lights[i / spot_light_fs_num_float];
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

		GL_StateManager::BindBuffer(GL_SHADER_STORAGE_BUFFER, m_spotlight_ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * light_array.size(), &light_array[0], GL_DYNAMIC_DRAW);
	}
}