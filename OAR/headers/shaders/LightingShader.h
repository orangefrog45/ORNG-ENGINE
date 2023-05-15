#pragma once
#include "shaders/Shader.h"
#include "rendering/Material.h"

namespace ORNG {
	class PointLightComponent;
	class SpotLightComponent;
	class DirectionalLight;
	class BaseLight;


	class LightingShader : public Shader {
	public:
		friend class ShaderLibrary;
		LightingShader() : Shader("lighting", 0) {
			AddStage(GL_VERTEX_SHADER, "res/shaders/LightingVS.shader");
			AddStage(GL_FRAGMENT_SHADER, "res/shaders/LightingFS.shader");
		}
		void InitUniforms();
		void SetAmbientLight(const BaseLight& light);
		void SetPointLights(std::vector<  PointLightComponent*>& p_lights);
		void SetSpotLights(std::vector< SpotLightComponent*>& s_lights);
		void SetDirectionLight(const DirectionalLight& light);

	private:
		static const unsigned int point_light_fs_num_float = 16;
		static const unsigned int spot_light_fs_num_float = 36;

	};

}