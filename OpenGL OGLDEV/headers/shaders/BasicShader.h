#pragma once
#include <glm/glm.hpp>
#include "Shader.h"

class BasicShader : public Shader {
public:
	void Init() override;
	void ActivateProgram() override;
	const GLint& GetProjectionLocation();
	const GLint& GetCameraLocation();
	const GLint& GetSamplerLocation();
	void SetProjection(const glm::fmat4& proj);
	void SetCamera(const glm::fmat4& cam);
private:
	void InitUniforms() override;
	unsigned int vert_shader_id;
	unsigned int frag_shader_id;
	unsigned int programID;
	GLint projectionLocation;
	GLint cameraLocation;
	GLint samplerLocation;
};
