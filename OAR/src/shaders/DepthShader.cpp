#include <glew.h>
#include <glfw3.h>
#include <DepthShader.h>

void DepthShader::InitUniforms() {
	ActivateProgram();
	m_pv_matrix_loc = GetUniform("pv_matrix");
}

void DepthShader::SetPVMatrix(const glm::mat4& mat) {
	glUniformMatrix4fv(m_pv_matrix_loc, 1, GL_FALSE, &mat[0][0]);
};