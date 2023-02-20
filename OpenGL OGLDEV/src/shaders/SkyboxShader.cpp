#include <glew.h>
#include <freeglut.h>
#include "shaders/BasicShader.h"
#include <iostream>
#include "shaders/SkyboxShader.h"
#include "util.h"

void SkyboxShader::Init() {
	GLCall(unsigned int tprogramID = glCreateProgram());

	CompileShader(GL_VERTEX_SHADER, ParseShader("res/shaders/SkyboxVS.shader"), vert_shader_id);
	CompileShader(GL_FRAGMENT_SHADER, ParseShader("res/shaders/SkyboxFS.shader"), frag_shader_id);

	UseShader(vert_shader_id, tprogramID);
	UseShader(frag_shader_id, tprogramID);

	SetProgramID(tprogramID);

	InitUniforms();

}

void SkyboxShader::ActivateProgram() {
	GLCall(glLinkProgram(GetProgramID()));
	GLCall(glValidateProgram(GetProgramID()));
	GLCall(glUseProgram(GetProgramID()));

	GLCall(glDeleteShader(vert_shader_id));
	GLCall(glDeleteShader(frag_shader_id));

}

void SkyboxShader::InitUniforms() {
	ActivateProgram();
	GLCall(WVPLocation = glGetUniformLocation(GetProgramID(), "gTransform"));
	GLCall(samplerLocation = glGetUniformLocation(GetProgramID(), "gSampler"));
}

const GLint& SkyboxShader::GetWVPLocation() {
	return WVPLocation;
}

const GLint& SkyboxShader::GetSamplerLocation() {
	return samplerLocation;
}


