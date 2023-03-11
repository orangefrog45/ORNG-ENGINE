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
	projectionLocation = GetUniform("projection");
	cameraLocation = GetUniform("camera");
	camera_pos_location = GetUniform("camera_pos");
}


const GLint& GridShader::GetProjectionLocation() const {
	return projectionLocation;
}

void GridShader::SetCamera(const glm::fmat4& cam) {
	glUniformMatrix4fv(GetCameraLocation(), 1, GL_TRUE, &cam[0][0]);
}

void GridShader::SetProjection(const glm::fmat4& proj) {
	glUniformMatrix4fv(GetProjectionLocation(), 1, GL_TRUE, &proj[0][0]);
}



const GLint& GridShader::GetCameraLocation() const {
	return cameraLocation;
}

const GLint& GridShader::GetSamplerLocation() const {
	return samplerLocation;
}


