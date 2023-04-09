#pragma once
#include "glm/vec3.hpp"
#include "glm/mat3x3.hpp"
#include "Shader.h"

class BasicSampler : public Shader {
public:
	BasicSampler() { paths.emplace_back("res/shaders/BasicSamplerVS.shader"); paths.emplace_back("res/shaders/BasicSamplerFS.shader"); }
	void InitUniforms() final;
	void SetTransform(const glm::mat3& transform);
	void SetCameraPos(const glm::vec3 pos);
	void SetCameraDir(const glm::vec3 dir);
private:
	int m_transform_loc = -1;
	int m_texture_sampler_loc = -1;
	int m_position_sampler_loc = -1;
	int m_depth_sampler_loc = -1;
	int m_camera_pos_loc = -1;
	int m_camera_dir_loc = -1;
};
