#include "BasicSampler.h"
#include "RendererData.h"

void BasicSampler::InitUniforms() {
	ActivateProgram();
	m_texture_sampler_loc = GetUniform("texture1");
	m_transform_loc = GetUniform("transform");
	glUniform1i(m_texture_sampler_loc, RendererData::TextureUnitIndexes::COLOR);
}

void BasicSampler::SetTransform(const glm::fmat3& transform) {
	glUniformMatrix3fv(m_transform_loc, 1, GL_TRUE, &transform[0][0]);
}