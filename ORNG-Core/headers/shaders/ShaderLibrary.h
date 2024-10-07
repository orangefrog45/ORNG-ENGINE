#pragma once
#include "Shader.h"


namespace ORNG {
	class DirectionalLight;
	struct PointLightComponent;
	class SpotLightComponent;

	class ShaderLibrary {
	public:
		friend class Renderer;
		ShaderLibrary() = default;
		void Init();

		Shader& CreateShader(const char* name);
		ShaderVariants& CreateShaderVariants(const char* name);

		bool GenerateShaderPackage(const std::string& output_filepath);

		bool ShaderPackageIsLoaded() {
			return !m_shader_package_cache.empty();
		}

		/* 
		Loads a shader package and stores shader code in a structure that can be referenced with the same filepaths used to create the package
		Shaders will look in this structure first when being created and prioritize using that code over searching the filesystem
		Default and expected behaviour during game runtime
		*/
		void LoadShaderPackage(const std::string& package_filepath);

		std::string PopShaderCodeFromCache(const ShaderData& key);

		void SetMatrixUBOs(const glm::mat4& proj, const glm::mat4& view);
		void SetGlobalLighting(const DirectionalLight& dir_light);
		void SetCommonUBO(glm::vec3 camera_pos, glm::vec3 camera_target, glm::vec3 cam_right, glm::vec3 cam_up, unsigned int render_resolution_x, unsigned int render_resolution_y, 
			float cam_zfar, float cam_znear, glm::vec3 voxel_aligned_cam_pos_c0, glm::vec3 voxel_aligned_cam_pos_c1, float scene_time_elapsed
		);

		Shader& GetShader(const std::string& name);

		void DeleteShader(const std::string& name);

		Shader& GetQuadShader() {
			return GetShader("SL quad");
		}

		void ReloadShaders();

		inline static const uint64_t LIGHTING_SHADER_ID = 1;
		inline static const uint64_t INVALID_SHADER_ID = 0; //useful for rendering things that should not have any shader applied to them (e.g skybox), only default gbuffer albedo
	private:

		std::unordered_map<ShaderData, std::string, ShaderData::Hash> m_shader_package_cache;

		std::unordered_map<std::string, Shader> m_shaders;
		std::unordered_map<std::string, ShaderVariants> m_shader_variants;

		UBO m_matrix_ubo{ true, 0 };
		inline const static unsigned int m_matrix_ubo_size = sizeof(glm::mat4) * 6;

		UBO m_global_lighting_ubo{ true, 0 };
		inline const static unsigned int m_global_lighting_ubo_size = 16 * sizeof(float);

		UBO m_common_ubo{true, 0};
		inline const static unsigned int m_common_ubo_size = sizeof(glm::vec4) * 8 + sizeof(float) * 7;
	};
}