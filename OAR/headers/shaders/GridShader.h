#pragma once
#include <glm/vec3.hpp>
#include "Shader.h"

class GridShader : public Shader {
public:
	GridShader() { paths.emplace_back("res/shaders/GridVS.shader"); paths.emplace_back("res/shaders/GridFS.shader"); }
	const unsigned int& GetSamplerLocation() const;
	void SetCameraPos(const glm::vec3 pos);
private:
	void InitUniforms() override;
	unsigned int m_camera_pos_location;
	unsigned int m_sampler_location;
};
