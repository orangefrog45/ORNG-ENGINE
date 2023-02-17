#include <glew.h>
#include <freeglut.h>
#include "shaders/BasicShader.h"
#include <iostream>
#include "shaders/SkyboxShader.h"
#include "util.h"

void SkyboxShader::Init() {
	unsigned int vertID;
	unsigned int fragID;
	GLCall(unsigned int tprogramID = glCreateProgram());

	CompileShader(GL_VERTEX_SHADER, ParseShader("res/shaders/SkyboxVS.shader"), vertID);
	CompileShader(GL_FRAGMENT_SHADER, ParseShader("res/shaders/SkyboxFS.shader"), fragID);

	UseShader(vertID, tprogramID);
	UseShader(fragID, tprogramID);

	SetProgramID(tprogramID);

	InitUniforms();

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


