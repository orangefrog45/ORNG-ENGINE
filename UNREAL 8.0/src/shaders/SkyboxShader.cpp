#include <glew.h>
#include <glfw/glfw3.h>
#include <iostream>
#include "shaders/SkyboxShader.h"
#include "util/util.h"
#include "RendererResources.h"

void SkyboxShader::InitUniforms() {
	ActivateProgram();
	sampler_location = GetUniform("gSampler");
	glUniform1i(sampler_location, RendererResources::TextureUnitIndexes::COLOR);
}



