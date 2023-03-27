#include <glew.h>
#include <glfw/glfw3.h>
#include "shaders/GridShader.h"
#include <iostream>
#include <glm/glm.hpp>
#include "util/util.h"

void GridShader::SetCameraPos(const glm::fvec3 pos) {
	glUniform3f(camera_pos_location, pos.x, pos.y, pos.z);
}

void GridShader::InitUniforms() {
	ActivateProgram();
	camera_pos_location = GetUniform("camera_pos");
}


const GLint& GridShader::GetSamplerLocation() const {
	return samplerLocation;
}


