#include "BasicSampler.h"

void BasicSampler::InitUniforms() {
	ActivateProgram();
	m_texture_sampler_loc = GetUniform("texture1");
	glUniform1i(m_texture_sampler_loc, TextureUnitIndexes::COLOR_TEXTURE_UNIT_INDEX);
}