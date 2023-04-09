#include "shaders/SkyboxShader.h"
#include "RendererResources.h"

void SkyboxShader::InitUniforms() {
	ActivateProgram();
	sampler_location = GetUniform("gSampler");
	glUniform1i(sampler_location, RendererResources::TextureUnitIndexes::COLOR);
}



