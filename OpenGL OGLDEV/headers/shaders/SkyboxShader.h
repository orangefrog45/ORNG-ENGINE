#pragma once
#include <Shader.h>
class SkyboxShader : public Shader {
public:
	void Init();
	void ActivateProgram();
	const GLint& GetWVPLocation();
	const GLint& GetSamplerLocation();
private:
	void InitUniforms();
	unsigned int vert_shader_id;
	unsigned int frag_shader_id;
	unsigned int programID;
	GLint WVPLocation;
	GLint samplerLocation;
};