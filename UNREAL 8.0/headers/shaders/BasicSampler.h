#pragma once
#include "Shader.h"

class BasicSampler : public Shader {
public:
	BasicSampler() { paths.emplace_back("res/shaders/BasicSamplerVS.shader"); paths.emplace_back("res/shaders/BasicSamplerFS.shader"); }
	void InitUniforms() final;
	void SetTransform(const glm::fmat3& transform);
private:
	unsigned int m_transform_loc = -1;
	unsigned int m_texture_sampler_loc = -1;
};
