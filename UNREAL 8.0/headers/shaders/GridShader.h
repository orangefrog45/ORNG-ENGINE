#pragma once
#include <glm/glm.hpp>
#include "Shader.h"

class GridShader : public Shader {
public:
	GridShader() { paths.push_back("res/shaders/GridVS.shader"); paths.push_back("res/shaders/GridFS.shader"); }
	const GLint& GetProjectionLocation() const;
	const GLint& GetCameraLocation() const;
	const GLint& GetSamplerLocation() const;
	void SetProjection(const glm::fmat4& proj);
	void SetCameraPos(const glm::fvec3 pos);
	void SetCamera(const glm::fmat4& cam);
private:
	void InitUniforms() override;
	GLint camera_pos_location;
	GLint projectionLocation;
	GLint cameraLocation;
	GLint samplerLocation;
};
