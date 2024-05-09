#include "pch/pch.h"

#include "shaders/ShaderLibrary.h"
#include "util/util.h"
#include "core/GLStateManager.h"
#include "core/FrameTiming.h"
#include "components/Lights.h"


namespace ORNG {
	void ShaderLibrary::Init() {
		m_matrix_ubo.Init();
		m_matrix_ubo.Resize(m_matrix_ubo_size);
		glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::PVMATRICES, m_matrix_ubo.GetHandle());

		m_global_lighting_ubo.Init();
		m_global_lighting_ubo.Resize(72 * sizeof(float));
		glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::GLOBAL_LIGHTING, m_global_lighting_ubo.GetHandle());

		m_common_ubo.Init();
		m_common_ubo.Resize(m_common_ubo_size);
		glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::GLOBALS, m_common_ubo.GetHandle());

		ORNG_CORE_TRACE(std::filesystem::current_path().string());
		mp_quad_shader = &CreateShader("SL quad");
		mp_quad_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/QuadVS.glsl");
		mp_quad_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/QuadFS.glsl");
		mp_quad_shader->Init();
	}

	void ShaderLibrary::SetCommonUBO(glm::vec3 camera_pos, glm::vec3 camera_target, glm::vec3 cam_right, glm::vec3 cam_up, unsigned int render_resolution_x, unsigned int render_resolution_y,
		float cam_zfar, float cam_znear, glm::vec3 voxel_aligned_cam_pos_c0, glm::vec3 voxel_aligned_cam_pos_c1, float scene_time_elapsed) {
		std::array<std::byte, m_common_ubo_size> data;
		std::byte* p_byte = data.data();

		// Any 0's are padding, all vec3 types are defined as vec4's in the shader for easier alignment.
		ConvertToBytes(p_byte,
			camera_pos, 0,
			camera_target, 0,
			cam_right, 0,
			cam_up, 0,
			voxel_aligned_cam_pos_c0, 0,
			voxel_aligned_cam_pos_c1, 0,
			voxel_aligned_cam_pos_c1, 0,
			voxel_aligned_cam_pos_c1, 0,
			static_cast<float>(FrameTiming::GetTotalElapsedTime()),
			static_cast<float>(render_resolution_x),
			static_cast<float>(render_resolution_y),
			cam_zfar,
			cam_znear,
			static_cast<float>(FrameTiming::GetTimeStep()),
			scene_time_elapsed
		);

		m_common_ubo.BufferSubData(0, m_common_ubo_size, data.data());
	}



	void ShaderLibrary::SetGlobalLighting(const DirectionalLight& dir_light) {
		constexpr int buffer_size = 72 * sizeof(float);
		std::array<std::byte, buffer_size> light_data;

		std::byte* p_byte = light_data.data();
		ConvertToBytes(p_byte,
			dir_light.GetLightDirection(),
			0,
			dir_light.color,
			0,
			dir_light.cascade_ranges,
			0,
			dir_light.light_size,
			dir_light.blocker_search_size,
			0,
			0,
			dir_light.m_light_space_matrices,
			dir_light.shadows_enabled,
			0,
			0,
			0
			);

		m_global_lighting_ubo.BufferSubData(0, buffer_size, light_data.data());
	};

	Shader& ShaderLibrary::CreateShader(const char* name) {
		if (m_shaders.contains(name)) {
			ORNG_CORE_ERROR("Shader name '{0}' already exists! Pick another name.", name);
			BREAKPOINT;
		}
		m_shaders.try_emplace(name, name);

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
		static std::vector<std::byte> matrices;
		matrices.resize(m_matrix_ubo_size);
		std::byte* p_byte = matrices.data();

		ConvertToBytes(p_byte,
			proj,
			view,
			proj_view,
			glm::inverse(proj),
			glm::inverse(view),
			glm::inverse(proj_view)
		);

		m_matrix_ubo.BufferSubData(0, m_matrix_ubo_size, matrices.data());
		glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::PVMATRICES, m_matrix_ubo.GetHandle());

	}
}