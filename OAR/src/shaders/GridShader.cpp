#include "shaders/GridShader.h"
#include <glew.h>
#include <glfw3.h>

void GridShader::SetCameraPos(const glm::vec3 pos) {
	glUniform3f(m_camera_pos_location, pos.x, pos.y, pos.z);
}

void GridShader::InitUniforms() {
	ActivateProgram();
	m_camera_pos_location = GetUniform("camera_pos");
}


const unsigned int& GridShader::GetSamplerLocation() const {
	return m_sampler_location;
}


