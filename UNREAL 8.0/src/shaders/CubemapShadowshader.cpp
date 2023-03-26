#include "CubemapShadowShader.h"

void CubemapShadowShader::InitUniforms() {
	ActivateProgram();
	m_light_pos_location = GetUniform("light_pos");
	m_pv_matrix_loc = GetUniform("pv_mat");
}