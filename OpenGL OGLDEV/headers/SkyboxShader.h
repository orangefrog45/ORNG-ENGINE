#pragma once
#include <glm/glm.hpp>
#include "Shader.h"

class SkyboxShader : public Shader {
public:
	void InitUniforms();
	void SendUniforms(const glm::fmat4& WVP);
private:
	unsigned int WVPLocation;
	unsigned int samplerLocation;
};
