#pragma once
#include "shaders/LightingShader.h"
#include "rendering/Material.h"
#include "components/lights/DirectionalLight.h"
#include "components/lights/BaseLight.h"


namespace ORNG {

	class ShaderLibrary {
	public:
		friend class Renderer;
		ShaderLibrary() {};
		void Init();

		/* Give shader filepaths in order: Vertex, Fragment */
		Shader& CreateShader(const char* name);
		void SetMatrixUBOs(glm::mat4& proj, glm::mat4& view);

		/* Sets material for gbuffer shader */
		void SetGBufferMaterial(const Material& material);
		void UpdateMaterialUBO(const std::vector<Material*>& materials);
		void SetGlobalLighting(const DirectionalLight& dir_light, const BaseLight& ambient_light);

		Shader& GetShader(const char* name);
		void DeleteShader(const char* name);

		unsigned int GetPointlightSSBOHandle() const { return m_pointlight_ssbo; }
		unsigned int GetSpotlightSSBOHandle() const { return m_spotlight_ssbo; }

		LightingShader lighting_shader;

		const unsigned int INVALID_SHADER_ID = 0; //useful for rendering things that should not have any shader applied to them (e.g skybox), only default gbuffer albedo
	private:
		[[nodiscard]] unsigned int CreateShaderID() { return m_last_id++; };


		std::unordered_map<std::string, Shader> m_shaders;
		unsigned int m_pointlight_ssbo;
		unsigned int m_spotlight_ssbo;
		unsigned int m_last_id = 2; // 0 = invalid shader, 1 = lighting shader
		unsigned int m_matrix_ubo;
		unsigned int m_material_ubo;
		unsigned int m_global_lighting_ubo; //contains things such as directional, ambient light as these are needed in a few shaders
	};

}