#include <glm/glm.hpp>
#include <glew.h>
#include "BasicShader.h"
#include "util.h"

void BasicShader::InitUniforms() {
	GLCall(WVPLocation = glGetUniformLocation(GetProgramID(), "gTransform"));
	ASSERT(WVPLocation != -1);
	GLCall(samplerLocation = glGetUniformLocation(GetProgramID(), "gSampler"));
	ASSERT(samplerLocation != -1);
}
