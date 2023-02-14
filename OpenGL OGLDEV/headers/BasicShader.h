#pragma once
#include "Shader.h"
#include <glm/glm.hpp>
class BasicShader : public Shader {
public:
	void InitUniforms();
private:
	unsigned int WVPLocation;
	unsigned int samplerLocation;
};