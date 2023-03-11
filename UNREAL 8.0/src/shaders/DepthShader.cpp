#include <DepthShader.h>

void DepthShader::InitUniforms() {
	ActivateProgram();
	m_pv_matrix_loc = GetUniform("pv_matrix");
}