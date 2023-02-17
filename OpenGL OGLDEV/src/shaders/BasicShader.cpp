#include <glew.h>
#include <freeglut.h>
#include "shaders/BasicShader.h"
#include <iostream>
#include "util.h"

void BasicShader::Init() {
	unsigned int vertID;
	unsigned int fragID;
	GLCall(unsigned int tprogramID = glCreateProgram());

	CompileShader(GL_VERTEX_SHADER, ParseShader("res/shaders/BasicVS.shader"), vertID);
	CompileShader(GL_FRAGMENT_SHADER, ParseShader("res/shaders/BasicFS.shader"), fragID);

	UseShader(vertID, tprogramID);
	UseShader(fragID, tprogramID);

	SetProgramID(tprogramID);

	//store uniforms only once
	InitUniforms();

}

void BasicShader::InitUniforms() {
	ActivateProgram();
	GLCall(WVPLocation = glGetUniformLocation(GetProgramID(), "gTransform"));
	GLCall(samplerLocation = glGetUniformLocation(GetProgramID(), "gSampler"));
}

const GLint& BasicShader::GetWVPLocation() {
	return WVPLocation;
}

const GLint& BasicShader::GetSamplerLocation() {
	return samplerLocation;
}


