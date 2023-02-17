#pragma once
#include <Shader.h>
class SkyboxShader : public Shader {
public:
	void Init();
	const GLint& GetWVPLocation();
	const GLint& GetSamplerLocation();
private:
	void InitUniforms();
	unsigned int programID;
	GLint WVPLocation;
	GLint samplerLocation;
};