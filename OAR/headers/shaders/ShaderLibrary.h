#pragma once
#include "rendering/Material.h"
#include "Shader.h"
#include "components/lights/DirectionalLight.h"
#include "components/lights/BaseLight.h"
#include "components/lights/PointLightComponent.h"

namespace ORNG {

	class PointLightComponent;
	class SpotLightComponent;

	class ShaderLibrary {
	public:
		friend class Renderer;
		ShaderLibrary() {};
		void Init();

		Shader& CreateShader(const char* name);
		void SetMatrixUBOs(glm::mat4& proj, glm::mat4& view);
		void UpdateMaterialUBO(const std::vector<Material*>& materials);
		void SetGlobalLighting(const DirectionalLight& dir_light, const BaseLight& ambient_light);
		void SetSpotLights(const std::vector<SpotLightComponent*>& s_lights);
		void SetPointLights(const std::vector<PointLightComponent*>& p_lights);
		void SetCommonUBO(glm::vec3 camera_pos, glm::vec3 camera_target);

		Shader& GetShader(const char* name);
		void DeleteShader(const char* name);

		unsigned int GetPointlightSSBOHandle() const { return m_pointlight_ssbo; }
		unsigned int GetSpotlightSSBOHandle() const { return m_spotlight_ssbo; }

		inline static const unsigned int LIGHTING_SHADER_ID = 1;
		inline static const unsigned int INVALID_MATERIAL_ID = 0;
		inline static const unsigned int INVALID_SHADER_ID = 0; //useful for rendering things that should not have any shader applied to them (e.g skybox), only default gbuffer albedo
	private:
		[[nodiscard]] unsigned int CreateShaderID() { return m_last_id++; };


		std::unordered_map<std::string, Shader> m_shaders;
		unsigned int m_pointlight_ssbo;
		unsigned int m_spotlight_ssbo;


		unsigned int m_matrix_ubo;
		inline const static unsigned int m_matrix_ubo_size = sizeof(glm::mat4) * 3;

		unsigned int m_material_ubo;

		unsigned int m_global_lighting_ubo; //contains things such as directional, ambient light
		inline const static unsigned int m_global_lighting_ubo_size = 18 * sizeof(float);


		unsigned int m_common_ubo;
		inline const static unsigned int m_common_ubo_size = sizeof(glm::vec4) * 2;


		unsigned int m_last_id = 2; // 0 = invalid shader, 1 = lighting shader
	};

}