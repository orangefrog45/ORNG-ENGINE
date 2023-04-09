#pragma once
#include <glm/vec3.hpp>
#include "Shader.h"

class ReflectionShader : public Shader {
public:
	ReflectionShader() { paths = { "./res/shaders/ReflectionVS.shader", "./res/shaders/ReflectionFS.shader" }; }
	void InitUniforms() final;
	void SetCameraPos(const glm::vec3& pos);
private:
	unsigned int camera_pos_loc;
};