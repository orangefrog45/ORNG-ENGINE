#pragma once
#include <Shader.h>
class SkyboxShader : public Shader {
public:
	void Init();
	void ActivateProgram();
	const GLint& GetWorldTransformLocation() const;
	const GLint& GetSamplerLocation() const;
private:
	void InitUniforms();
	unsigned int vert_shader_id;
	unsigned int frag_shader_id;
	unsigned int programID;
	GLint m_world_transform_loc;
	GLint samplerLocation;
};