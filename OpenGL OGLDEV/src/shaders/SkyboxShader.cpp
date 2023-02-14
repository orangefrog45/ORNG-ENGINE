#include <glm/glm.hpp>
#include <glew.h>
#include "SkyboxShader.h"
#include "util.h"

void SkyboxShader::InitUniforms() {
	GLCall(WVPLocation = glGetUniformLocation(GetProgramID(), "gTransform"));
	ASSERT(WVPLocation != -1);
	GLCall(samplerLocation = glGetUniformLocation(GetProgramID(), "gSampler"));
	ASSERT(samplerLocation != -1);
}

