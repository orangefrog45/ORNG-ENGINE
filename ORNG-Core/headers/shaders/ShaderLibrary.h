#pragma once
#include "Shader.h"


namespace ORNG {
	class DirectionalLight;
	struct PointLightComponent;
	struct SpotLightComponent;

	class ShaderLibrary {
	public:
		friend class Renderer;
		ShaderLibrary() = default;
		void Init();

		// Create a shader, optionally with an ID (currently only used with in-built shaders)
		Shader& CreateShader(const char* name, unsigned int id = 0);
		ShaderVariants& CreateShaderVariants(const char* name);

		void SetMatrixUBOs(glm::mat4& proj, glm::mat4& view);
		void SetGlobalLighting(const DirectionalLight& dir_light);
		void SetCommonUBO(glm::vec3 camera_pos, glm::vec3 camera_target, unsigned int render_resolution_x, unsigned int render_resolution_y, float cam_zfar, float cam_znear);

		Shader& GetShader(const char* name);
		void DeleteShader(const char* name);

		void ReloadShaders();

		[[nodiscard]] inline unsigned int CreateIncrementalShaderID() const { // This can be incremental as these id's don't need to stay the same between
			static unsigned int last_id = 2;								  // different invocations of this program except the hard-coded ones
			return last_id++;
		}

		inline static const uint64_t LIGHTING_SHADER_ID = 1;
		inline static const uint64_t INVALID_SHADER_ID = 0; //useful for rendering things that should not have any shader applied to them (e.g skybox), only default gbuffer albedo
	private:

		std::unordered_map<std::string, Shader> m_shaders;
		std::unordered_map<std::string, ShaderVariants> m_shader_variants;

		unsigned int m_matrix_ubo;
		inline const static unsigned int m_matrix_ubo_size = sizeof(glm::mat4) * 5;

		unsigned int m_global_lighting_ubo; // Currently just contains directional light data, size is rounded from 13 to 16 for alignment
		inline const static unsigned int m_global_lighting_ubo_size = 16 * sizeof(float);


		unsigned int m_common_ubo;
		inline const static unsigned int m_common_ubo_size = sizeof(glm::vec4) * 2 + sizeof(float) * 6;
	};
}