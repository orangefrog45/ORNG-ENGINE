#include <glew.h>
#include <freeglut.h>
#include "shaders/GridShader.h"
#include <iostream>
#include <glm/glm.hpp>
#include "util.h"

void GridShader::Init() {
	GLCall(unsigned int tprogramID = glCreateProgram());

	CompileShader(GL_VERTEX_SHADER, ParseShader("res/shaders/GridVS.shader"), vert_shader_id);
	CompileShader(GL_FRAGMENT_SHADER, ParseShader("res/shaders/GridFS.shader"), frag_shader_id);

	UseShader(vert_shader_id, tprogramID);
	UseShader(frag_shader_id, tprogramID);

	SetProgramID(tprogramID);

	//store uniforms only once
	InitUniforms();

}

void GridShader::ActivateProgram() {
	GLCall(glLinkProgram(GetProgramID()));
	GLCall(glValidateProgram(GetProgramID()));
	GLCall(glUseProgram(GetProgramID()));

	GLCall(glDeleteShader(vert_shader_id));
	GLCall(glDeleteShader(frag_shader_id));
}

void GridShader::SetCameraPos(const glm::fvec3 pos) {
	glUniform3f(camera_pos_location, pos.x, pos.y, pos.z);
}

void GridShader::InitUniforms() {
	ActivateProgram();
	projectionLocation = GetUniform("projection");
	cameraLocation = GetUniform("camera");
	camera_pos_location = GetUniform("camera_pos");
}


const GLint& GridShader::GetProjectionLocation() {
	return projectionLocation;
}

void GridShader::SetCamera(const glm::fmat4& cam) {
	glUniformMatrix4fv(this->GetCameraLocation(), 1, GL_TRUE, &cam[0][0]);
}

void GridShader::SetProjection(const glm::fmat4& proj) {
	glUniformMatrix4fv(this->GetProjectionLocation(), 1, GL_TRUE, &proj[0][0]);
}



const GLint& GridShader::GetCameraLocation() {
	return cameraLocation;
}

const GLint& GridShader::GetSamplerLocation() {
	return samplerLocation;
}


