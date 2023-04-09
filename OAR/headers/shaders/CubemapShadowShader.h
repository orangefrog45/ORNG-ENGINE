#pragma once
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "Shader.h"


class CubemapShadowShader : public Shader {
public:
	CubemapShadowShader() { paths = { "res/shaders/CubemapShadowVS.shader", "res/shaders/CubemapShadowFS.shader" }; }
	void InitUniforms() final;
	void SetLightPos(glm::vec3 pos);
	void SetPVMatrix(const glm::mat4& mat);
private:
	int m_pv_matrix_loc = -1;
	int m_light_pos_location = -1;
};