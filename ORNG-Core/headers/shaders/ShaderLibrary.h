#pragma once
#include "Shader.h"


namespace ORNG {
	// Contains a collection of common/useful shaders
	class ShaderLibrary {
	public:
		friend class Renderer;
		ShaderLibrary() = default;

		void Init();

		Shader& GetQuadShader() {
			return m_quad_shader;
		}

		void ReloadShaders();

		inline static const uint64_t LIGHTING_SHADER_ID = 1;
		inline static const uint64_t INVALID_SHADER_ID = 0; //useful for rendering things that should not have any shader applied to them (e.g skybox), only default gbuffer albedo
	private:
		Shader m_quad_shader;
	};
}
