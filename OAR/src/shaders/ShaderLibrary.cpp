#include "pch/pch.h"

#include "shaders/ShaderLibrary.h"
#include "util/util.h"
#include "rendering/Renderer.h"
#include "core/GLStateManager.h"
#include "core/FrameTiming.h"


namespace ORNG {

	void ShaderLibrary::Init() {

		m_matrix_ubo = GL_StateManager::GenBuffer();
		GL_StateManager::BindBuffer(GL_UNIFORM_BUFFER, m_matrix_ubo);
		glBufferData(GL_UNIFORM_BUFFER, 3 * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::PVMATRICES, m_matrix_ubo);

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
		data[8] = FrameTiming::GetTotalElapsedTime();

		GL_StateManager::BindBuffer(GL_UNIFORM_BUFFER, m_common_ubo);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, m_common_ubo_size, &data[0]);
	}

	void ShaderLibrary::SetGlobalLighting(const DirectionalLight& dir_light) {
		GL_StateManager::BindBuffer(GL_UNIFORM_BUFFER, m_global_lighting_ubo);

		constexpr int num_floats_in_buffer = 12;
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
		light_data[8] = dir_light.cascade_ranges[0];
		light_data[9] = dir_light.cascade_ranges[1];
		light_data[10] = dir_light.cascade_ranges[2];
		light_data[11] = 0; //padding

		glBufferSubData(GL_UNIFORM_BUFFER, 0, num_floats_in_buffer * sizeof(float), &light_data[0]);
	};

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



}