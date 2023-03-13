#include <glew.h>
#include <glfw/glfw3.h>
#include <iostream>
#include "shaders/SkyboxShader.h"
#include "util/util.h"

void SkyboxShader::InitUniforms() {
	ActivateProgram();
	m_view_loc = GetUniform("gTransform");
	samplerLocation = GetUniform("gSampler");
	glUniform1i(samplerLocation, TextureUnitIndexes::COLOR_TEXTURE_UNIT_INDEX);
}

const GLint& SkyboxShader::GetViewLoc() const {
	return m_view_loc;
}

const GLint& SkyboxShader::GetSamplerLocation() const {
	return samplerLocation;
}


