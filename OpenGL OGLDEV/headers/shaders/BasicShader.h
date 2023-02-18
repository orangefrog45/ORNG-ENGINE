#pragma once
#include <glm/glm.hpp>
#include "Shader.h"

class BasicShader : public Shader {
public:
	void Init();
	void ActivateProgram();
	const GLint& GetProjectionLocation();
	const GLint& GetCameraLocation();
	const GLint& GetSamplerLocation();
	void SetProjection(const glm::fmat4& proj);
	void SetCamera(const glm::fmat4& cam);
private:
	void InitUniforms();
	unsigned int vert_shader_id;
	unsigned int frag_shader_id;
	unsigned int programID;
	GLint projectionLocation;
	GLint cameraLocation;
	GLint samplerLocation;
};
