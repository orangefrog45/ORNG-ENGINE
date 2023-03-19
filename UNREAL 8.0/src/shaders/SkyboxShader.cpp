#include <glew.h>
#include <glfw/glfw3.h>
#include <iostream>
#include "shaders/SkyboxShader.h"
#include "util/util.h"
#include "RendererData.h"

void SkyboxShader::InitUniforms() {
	ActivateProgram();
	m_view_loc = GetUniform("gTransform");
	samplerLocation = GetUniform("gSampler");
	glUniform1i(samplerLocation, RendererData::TextureUnitIndexes::COLOR);
}

const GLint& SkyboxShader::GetViewLoc() const {
	return m_view_loc;
}

const GLint& SkyboxShader::GetSamplerLocation() const {
	return samplerLocation;
}


