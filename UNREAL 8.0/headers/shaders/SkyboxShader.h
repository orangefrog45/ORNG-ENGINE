#pragma once
#include <Shader.h>
class SkyboxShader : public Shader {
public:
	SkyboxShader() { paths.emplace_back("res/shaders/SkyboxVS.shader"); paths.emplace_back("res/shaders/SkyboxFS.shader"); }
	const GLint& GetViewLoc() const;
	const GLint& GetSamplerLocation() const;
private:
	void InitUniforms() final;
	unsigned int vert_shader_id;
	unsigned int frag_shader_id;
	unsigned int programID;
	GLint m_view_loc;
	GLint samplerLocation;
};