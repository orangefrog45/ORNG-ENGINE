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

void BasicSampler::SetTransform(const glm::mat3& transform) {
	glUniformMatrix3fv(m_transform_loc, 1, GL_TRUE, &transform[0][0]);
};

void BasicSampler::SetCameraPos(const glm::vec3 pos) {
	glUniform3f(m_camera_pos_loc, pos.x, pos.y, pos.z);
};

void BasicSampler::SetCameraDir(const glm::vec3 dir) {
	glm::vec3 normal_dir = glm::normalize(dir);  glUniform3f(m_camera_dir_loc, normal_dir.x, normal_dir.y, normal_dir.z);
}
