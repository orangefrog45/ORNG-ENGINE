#include "pch/pch.h"

#include "shaders/ShaderLibrary.h"
#include "util/util.h"
#include "core/GLStateManager.h"
#include "core/FrameTiming.h"
#include "components/Lights.h"


namespace ORNG {
	void ShaderLibrary::Init() {
		m_matrix_ubo = GL_StateManager::GenBuffer();
		GL_StateManager::BindBuffer(GL_UNIFORM_BUFFER, m_matrix_ubo);
		glBufferData(GL_UNIFORM_BUFFER, m_matrix_ubo_size, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::PVMATRICES, m_matrix_ubo);

		m_global_lighting_ubo = GL_StateManager::GenBuffer();
		GL_StateManager::BindBuffer(GL_UNIFORM_BUFFER, m_global_lighting_ubo);
		glBufferData(GL_UNIFORM_BUFFER, 68 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::GLOBAL_LIGHTING, m_global_lighting_ubo);

		m_common_ubo = GL_StateManager::GenBuffer();
		GL_StateManager::BindBuffer(GL_UNIFORM_BUFFER, m_common_ubo);
		glBufferData(GL_UNIFORM_BUFFER, m_common_ubo_size, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::GLOBALS, m_common_ubo);

		mp_quad_shader = &CreateShader("SL quad");
		mp_quad_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/QuadVS.glsl");
		mp_quad_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/QuadFS.glsl");
		mp_quad_shader->Init();
	}

	void ShaderLibrary::SetCommonUBO(glm::vec3 camera_pos, glm::vec3 camera_target, glm::vec3 cam_right, glm::vec3 cam_up, unsigned int render_resolution_x, unsigned int render_resolution_y, float cam_zfar, float cam_znear) {
		std::array<std::byte, m_common_ubo_size> data;
		std::byte* p_byte = data.data();

		ConvertToBytes(camera_pos, p_byte);
		ConvertToBytes(0, p_byte);
		ConvertToBytes(camera_target, p_byte);
		ConvertToBytes(0, p_byte);
		ConvertToBytes(cam_right, p_byte);
		ConvertToBytes(0, p_byte);
		ConvertToBytes(cam_up, p_byte);
		ConvertToBytes(0, p_byte);
		ConvertToBytes((float)FrameTiming::GetTotalElapsedTime(), p_byte);
		ConvertToBytes(render_resolution_x, p_byte);
		ConvertToBytes(render_resolution_y, p_byte);
		ConvertToBytes(cam_zfar, p_byte);
		ConvertToBytes(cam_znear, p_byte);
		ConvertToBytes((float)FrameTiming::GetTimeStep(), p_byte);
		ConvertToBytes(FrameTiming::GetTotalElapsedTime(), p_byte);


		GL_StateManager::BindBuffer(GL_UNIFORM_BUFFER, m_common_ubo);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, m_common_ubo_size, &data[0]);
	}



	void ShaderLibrary::SetGlobalLighting(const DirectionalLight& dir_light) {
		GL_StateManager::BindBuffer(GL_UNIFORM_BUFFER, m_global_lighting_ubo);

		constexpr int num_floats_in_buffer = 68;
		std::array<float, num_floats_in_buffer> light_data = { 0 };
		int i = 0;
		glm::vec3 light_dir = dir_light.GetLightDirection();
		light_data[i++] = light_dir.x;
		light_data[i++] = light_dir.y;
		light_data[i++] = light_dir.z;
		light_data[i++] = 0; //padding
		light_data[i++] = dir_light.color.x;
		light_data[i++] = dir_light.color.y;
		light_data[i++] = dir_light.color.z;
		light_data[i++] = 0; //padding
		light_data[i++] = dir_light.cascade_ranges[0];
		light_data[i++] = dir_light.cascade_ranges[1];
		light_data[i++] = dir_light.cascade_ranges[2];
		light_data[i++] = 0; //padding
		light_data[i++] = dir_light.light_size;
		light_data[i++] = dir_light.blocker_search_size;
		light_data[i++] = 0; //padding
		light_data[i++] = 0; //padding
		PushMatrixIntoArray(dir_light.m_light_space_matrices[0], &light_data[i]); i += 16;
		PushMatrixIntoArray(dir_light.m_light_space_matrices[1], &light_data[i]); i += 16;
		PushMatrixIntoArray(dir_light.m_light_space_matrices[2], &light_data[i]); i += 16;
		light_data[i++] = dir_light.shadows_enabled;
		light_data[i++] = 0; //padding
		light_data[i++] = 0; //padding
		light_data[i++] = 0; //padding

		glBufferSubData(GL_UNIFORM_BUFFER, 0, num_floats_in_buffer * sizeof(float), &light_data[0]);
	};

	Shader& ShaderLibrary::CreateShader(const char* name, unsigned int id) {
		if (m_shaders.contains(name)) {
			ORNG_CORE_ERROR("Shader name '{0}' already exists! Pick another name.", name);
			BREAKPOINT;
		}
		if (id == 0)
			m_shaders.try_emplace(name, name, CreateIncrementalShaderID());
		else
			m_shaders.try_emplace(name, name, id);

		ORNG_CORE_INFO("Shader '{0}' created", name);
		return m_shaders[name];
	}

	ShaderVariants& ShaderLibrary::CreateShaderVariants(const char* name) {
		if (m_shaders.contains(name)) {
			ORNG_CORE_ERROR("Shader name '{0}' already exists! Pick another name.", name);
			BREAKPOINT;
		}
		m_shader_variants.try_emplace(name, name);

		ORNG_CORE_INFO("ShaderVariants '{0}' created", name);
		return m_shader_variants[name];
	}

	Shader& ShaderLibrary::GetShader(const char* name) {
		if (!m_shaders.contains(name)) {
			ORNG_CORE_CRITICAL("No shader with name '{0}' exists", name);
			BREAKPOINT;
		}
		return m_shaders[name];
	}

	void ShaderLibrary::ReloadShaders() {
		for (auto& [name, shader] : m_shaders) {
			shader.Reload();
		}

		for (auto& [name, sv] : m_shader_variants) {
			for (auto& [id, shader] : sv.m_shaders) {
				shader.Reload();
			}
		}
	}

	void ShaderLibrary::SetMatrixUBOs(glm::mat4& proj, glm::mat4& view) {
		glm::mat4 proj_view = proj * view;
		glBindBuffer(GL_UNIFORM_BUFFER, m_matrix_ubo);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &proj[0][0]);
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &view[0][0]);
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, sizeof(glm::mat4), &proj_view[0][0]);
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 3, sizeof(glm::mat4), &glm::inverse(proj)[0][0]);
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 4, sizeof(glm::mat4), &glm::inverse(view)[0][0]);
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 5, sizeof(glm::mat4), &glm::inverse(proj_view)[0][0]);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
}