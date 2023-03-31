#pragma once
#include <Shader.h>
class SkyboxShader : public Shader {
public:
	SkyboxShader() { paths.emplace_back("res/shaders/SkyboxVS.shader"); paths.emplace_back("res/shaders/SkyboxFS.shader"); }
private:
	void InitUniforms() final;
	unsigned int programID;
	GLint sampler_location;
};