#pragma once
#include <Shader.h>
#include "glm/mat4x4.hpp"
class DepthShader : public Shader {
public:
	DepthShader() { paths.emplace_back("res/shaders/DepthVS.shader"); paths.emplace_back("res/shaders/DepthFS.shader"); }
	void InitUniforms() final;
	void SetPVMatrix(const glm::mat4& mat);
private:
	unsigned int m_pv_matrix_loc;
};