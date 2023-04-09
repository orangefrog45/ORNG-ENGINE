#include <glew.h>
#include <glfw/glfw3.h>
#include "CubemapShadowShader.h"

void CubemapShadowShader::InitUniforms() {
	ActivateProgram();
	m_light_pos_location = GetUniform("light_pos");
	m_pv_matrix_loc = GetUniform("pv_mat");
}

inline void CubemapShadowShader::SetLightPos(glm::vec3 pos) {
	glUniform3f(m_light_pos_location, pos.x, pos.y, pos.z);
};

inline void CubemapShadowShader::SetPVMatrix(const glm::mat4& mat) {
	glUniformMatrix4fv(m_pv_matrix_loc, 1, GL_FALSE, &mat[0][0]);
}