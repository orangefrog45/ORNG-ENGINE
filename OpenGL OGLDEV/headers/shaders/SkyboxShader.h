#pragma once
#include <Shader.h>
class SkyboxShader : public Shader {
public:
	void Init();
	void InitUniforms();
	const GLint& GetWVPLocation();
	const GLint& GetSamplerLocation();
private:
	unsigned int programID;
	GLint WVPLocation;
	GLint samplerLocation;
};