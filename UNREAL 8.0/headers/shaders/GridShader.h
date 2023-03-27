#pragma once
#include <glm/glm.hpp>
#include "Shader.h"

class GridShader : public Shader {
public:
	GridShader() { paths.emplace_back("res/shaders/GridVS.shader"); paths.emplace_back("res/shaders/GridFS.shader"); }
	const GLint& GetSamplerLocation() const;
	void SetCameraPos(const glm::fvec3 pos);
private:
	void InitUniforms() override;
	GLint camera_pos_location;
	GLint samplerLocation;
};
