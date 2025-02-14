#pragma once
#include "rendering/VAO.h"
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

		Shader* GetShader(const std::string& name);

		void DeleteShader(const std::string& name);

		Shader& GetQuadShader() {
			return *GetShader("SL quad");
		}

		void ReloadShaders();

		inline static const uint64_t LIGHTING_SHADER_ID = 1;
		inline static const uint64_t INVALID_SHADER_ID = 0; //useful for rendering things that should not have any shader applied to them (e.g skybox), only default gbuffer albedo
	private:
		std::unordered_map<std::string, Shader> m_shaders;
		std::unordered_map<std::string, ShaderVariants> m_shader_variants;
	};
}