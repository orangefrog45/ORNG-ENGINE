#pragma once
#include "Shader.h"

class ReflectionShader : public Shader {
public:
	ReflectionShader() { paths = { "./res/shaders/ReflectionVS.shader", "./res/shaders/ReflectionFS.shader" }; }
	void InitUniforms() final { ActivateProgram(); camera_pos_loc = GetUniform("camera_pos"); };
	void SetCameraPos(const glm::fvec3& pos) {   glUniform3f(camera_pos_loc, pos.x, pos.y, pos.z); }
private:
	unsigned int camera_pos_loc;
};