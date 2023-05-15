#include "pch/pch.h"

#include "shaders/ShaderLibrary.h"
#include "util/util.h"
#include "rendering/Renderer.h"
#include "core/GLStateManager.h"


namespace ORNG {

	void ShaderLibrary::Init() {
		lighting_shader.Init();
		lighting_shader.InitUniforms();
		lighting_shader.m_shader_id = 1;

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
		glBufferData(GL_UNIFORM_BUFFER, 5 * sizeof(glm::vec4), nullptr, GL_STATIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::GLOBAL_LIGHTING, m_global_lighting_ubo);
	}

	void ShaderLibrary::SetGlobalLighting(const DirectionalLight& dir_light, const BaseLight& ambient_light) {
		GL_StateManager::BindBuffer(GL_UNIFORM_BUFFER, m_global_lighting_ubo);

		constexpr int num_floats_in_buffer = 18;
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
		light_data[10] = 0;
		light_data[11] = 0; //padding
		light_data[12] = ambient_light.color.x;
		light_data[13] = ambient_light.color.y;
		light_data[14] = ambient_light.color.z;
		light_data[15] = 0;
		light_data[16] = ambient_light.ambient_intensity;
		light_data[17] = ambient_light.diffuse_intensity;

		glBufferSubData(GL_UNIFORM_BUFFER, 0, num_floats_in_buffer * sizeof(float), &light_data[0]);
	};

	void ShaderLibrary::UpdateMaterialUBO(const std::vector<Material*>& materials) {

		constexpr int material_size_floats = 16;
		std::array<float, Renderer::max_materials* material_size_floats> material_arr = { 0 };
		unsigned int index = 0;

		for (const auto* material : materials) {
			material_arr[index] = material->ambient_color.x;
			index++;
			material_arr[index] = material->ambient_color.y;
			index++;
			material_arr[index] = material->ambient_color.z;
			index++;
			material_arr[index] = 0; //padding
			index++;

			material_arr[index] = material->diffuse_color.x;
			index++;
			material_arr[index] = material->diffuse_color.y;
			index++;
			material_arr[index] = material->diffuse_color.z;
			index++;
			material_arr[index] = 0; //padding
			index++;

			material_arr[index] = material->specular_color.x;
			index++;
			material_arr[index] = material->specular_color.y;
			index++;
			material_arr[index] = material->specular_color.z;
			index++;
			material_arr[index] = 0; //padding
			index++;
			material_arr[index] = material->normal_map_texture == nullptr ? 0 : 1; //if material is using normal maps
			index++;
			material_arr[index] = 0; //padding
			index++;
			material_arr[index] = 0; //padding
			index++;
			material_arr[index] = 0; //padding

		}

		glBindBuffer(GL_UNIFORM_BUFFER, m_material_ubo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(float) * material_arr.size(), &material_arr, GL_DYNAMIC_DRAW);

	}
	Shader& ShaderLibrary::CreateShader(const char* name) {
		m_shaders.try_emplace(name, name, CreateShaderID());
		OAR_CORE_INFO("Shader '{0}' created", name);
		return m_shaders[name];
	}

	void ShaderLibrary::SetGBufferMaterial(const Material& material)
	{
		Shader& gbuffer_shader = GetShader("gbuffer");

		if (material.diffuse_texture != nullptr) {
			GL_StateManager::BindTexture(GL_TEXTURE_2D, material.diffuse_texture->GetTextureHandle(), GL_StateManager::TextureUnits::COLOR, false);
		}

		if (material.specular_texture == nullptr) {
			gbuffer_shader.SetUniform("specular_sampler_active", 0);
		}
		else {
			gbuffer_shader.SetUniform("specular_sampler_active", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, material.specular_texture->GetTextureHandle(), GL_StateManager::TextureUnits::SPECULAR, false);
		}

		if (material.normal_map_texture == nullptr) {
			gbuffer_shader.SetUniform("u_normal_sampler_active", 0);
		}
		else {
			gbuffer_shader.SetUniform("u_normal_sampler_active", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, material.normal_map_texture->GetTextureHandle(), GL_StateManager::TextureUnits::NORMAL_MAP, false);
		}
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
}