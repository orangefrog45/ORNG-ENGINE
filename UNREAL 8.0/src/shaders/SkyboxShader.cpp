#include <glew.h>
#include <glfw/glfw3.h>
#include <iostream>
#include "shaders/SkyboxShader.h"
#include "util/util.h"

void SkyboxShader::InitUniforms() {
	ActivateProgram();
	m_world_transform_loc = GetUniform("gTransform");
	samplerLocation = GetUniform("gSampler");
	glUniform1i(samplerLocation, TextureUnitIndexes::COLOR_TEXTURE_UNIT_INDEX);
}

const GLint& SkyboxShader::GetWorldTransformLocation() const {
	return m_world_transform_loc;
}

const GLint& SkyboxShader::GetSamplerLocation() const {
	return samplerLocation;
}


