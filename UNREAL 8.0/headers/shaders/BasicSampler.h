#pragma once
#include "Shader.h"

class BasicSampler : public Shader {
public:
	BasicSampler() { paths.emplace_back("res/shaders/BasicSamplerVS.shader"); paths.emplace_back("res/shaders/BasicSamplerFS.shader"); }
	void InitUniforms() final;
	inline void SetTransform(const glm::fmat3& transform) { glUniformMatrix3fv(m_transform_loc, 1, GL_TRUE, &transform[0][0]); };
	inline void SetCameraPos(const glm::fvec3 pos) { glUniform3f(m_camera_pos_loc, pos.x, pos.y, pos.z); };
	inline void SetCameraDir(const glm::fvec3 dir) { glm::fvec3 normal_dir = glm::normalize(dir);  glUniform3f(m_camera_dir_loc, normal_dir.x, normal_dir.y, normal_dir.z); }
private:
	int m_transform_loc = -1;
	int m_texture_sampler_loc = -1;
	int m_position_sampler_loc = -1;
	int m_depth_sampler_loc = -1;
	int m_camera_pos_loc = -1;
	int m_camera_dir_loc = -1;
};
