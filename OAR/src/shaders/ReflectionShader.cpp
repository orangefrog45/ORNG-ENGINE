#include <glew.h>
#include <glfw3.h>
#include "ReflectionShader.h"

void ReflectionShader::InitUniforms() {
	ActivateProgram(); camera_pos_loc = GetUniform("camera_pos");
};

void ReflectionShader::SetCameraPos(const glm::vec3& pos) {
	glUniform3f(camera_pos_loc, pos.x, pos.y, pos.z);
}