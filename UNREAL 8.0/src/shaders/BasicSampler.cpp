#include "BasicSampler.h"
#include "RendererResources.h"

void BasicSampler::InitUniforms() {
	ActivateProgram();
	m_texture_sampler_loc = GetUniform("texture1");
	m_position_sampler_loc = GetUniform("world_positions");
	m_depth_sampler_loc = GetUniform("depth_map");
	m_transform_loc = GetUniform("transform");
	m_camera_pos_loc = GetUniform("camera_pos");
	m_camera_dir_loc = GetUniform("normalized_camera_dir");
	glUniform1i(m_texture_sampler_loc, RendererResources::TextureUnitIndexes::COLOR);
	glUniform1i(m_texture_sampler_loc, RendererResources::TextureUnitIndexes::WORLD_POSITIONS);
	glUniform1i(m_depth_sampler_loc, RendererResources::TextureUnitIndexes::DIR_SHADOW_MAP);
}
