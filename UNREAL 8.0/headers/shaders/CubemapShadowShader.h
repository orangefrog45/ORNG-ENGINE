#pragma once
#include "Shader.h"

class CubemapShadowShader : public Shader {
public:
	CubemapShadowShader() { paths = { "res/shaders/CubemapShadowVS.shader", "res/shaders/CubemapShadowFS.shader" }; }
	void InitUniforms() final;
	void SetLightPos(glm::fvec3 pos) { glUniform3f(m_light_pos_location, pos.x, pos.y, pos.z); };
	void SetPVMatrix(const glm::fmat4& mat) { glUniformMatrix4fv(m_pv_matrix_loc, 1, GL_FALSE, &mat[0][0]); }
private:
	int m_pv_matrix_loc = -1;
	int m_light_pos_location = -1;
};