#pragma once
#include <Shader.h>
class DepthShader : public Shader {
public:
	DepthShader() { paths.emplace_back("res/shaders/DepthVS.shader"); paths.emplace_back("res/shaders/DepthFS.shader"); }
	void InitUniforms() final;
	void SetPVMatrix(const glm::fmat4& mat) { glUniformMatrix4fv(m_pv_matrix_loc, 1, GL_FALSE, &mat[0][0]); };
private:
	GLint m_pv_matrix_loc;
};