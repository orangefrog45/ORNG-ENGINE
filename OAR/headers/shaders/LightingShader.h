#pragma once
#include "shaders/Shader.h"
#include "rendering/Material.h"

class PointLightComponent;
class SpotLightComponent;
class DirectionalLight;
class BaseLight;

class LightingShader : public Shader {
public:
	friend class ShaderLibrary;
	LightingShader() : Shader("lighting", 0) { paths = { "res/shaders/LightingVS.shader", "res/shaders/LightingFS.shader" }; }
	void Init();
	void InitUniforms();
	void SetViewPos(const glm::vec3& pos);
	void SetAmbientLight(const BaseLight& light);
	void SetPointLights(std::vector<  PointLightComponent*>& p_lights);
	void SetSpotLights(std::vector< SpotLightComponent*>& s_lights);
	void SetMaterial(const Material& material);
	void SetDirectionLight(const DirectionalLight& light);
	void GenUBOs();

private:
	static const unsigned int point_light_fs_num_float = 16;
	static const unsigned int spot_light_fs_num_float = 36;

};