#include <glew.h>
#include <freeglut.h>
#include "shaders/BasicShader.h"
#include <iostream>
#include <glm/glm.hpp>
#include "util.h"

void BasicShader::Init() {
	GLCall(unsigned int tprogramID = glCreateProgram());

	CompileShader(GL_VERTEX_SHADER, ParseShader("res/shaders/BasicVS.shader"), vert_shader_id);
	CompileShader(GL_FRAGMENT_SHADER, ParseShader("res/shaders/BasicFS.shader"), frag_shader_id);

	UseShader(vert_shader_id, tprogramID);
	UseShader(frag_shader_id, tprogramID);

	SetProgramID(tprogramID);

	//store uniforms only once
	InitUniforms();

}

void BasicShader::ActivateProgram() {
	GLCall(glLinkProgram(GetProgramID()));
	GLCall(glValidateProgram(GetProgramID()));
	GLCall(glUseProgram(GetProgramID()));

	GLCall(glDeleteShader(vert_shader_id));
	GLCall(glDeleteShader(frag_shader_id));
}

void BasicShader::InitUniforms() {
	ActivateProgram();
	GLCall(projectionLocation = glGetUniformLocation(GetProgramID(), "projection"));
	GLCall(cameraLocation = glGetUniformLocation(GetProgramID(), "camera"));
	GLCall(samplerLocation = glGetUniformLocation(GetProgramID(), "gSampler"));
}


const GLint& BasicShader::GetProjectionLocation() {
	return projectionLocation;
}

void BasicShader::SetCamera(const glm::fmat4& cam) {
	glUniformMatrix4fv(this->GetCameraLocation(), 1, GL_TRUE, &cam[0][0]);
}

void BasicShader::SetProjection(const glm::fmat4& proj) {
	glUniformMatrix4fv(this->GetProjectionLocation(), 1, GL_TRUE, &proj[0][0]);
}



const GLint& BasicShader::GetCameraLocation() {
	return cameraLocation;
}

const GLint& BasicShader::GetSamplerLocation() {
	return samplerLocation;
}


