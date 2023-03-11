#include <glew.h>
#include <glfw/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <format>
#include "Shader.h"


const GLint Shader::GetProgramID() {
	return m_programID;
}

Shader::~Shader() {
	GLCall(glDeleteProgram(GetProgramID()));
}

void Shader::SetProgramID(const GLint id) {
	m_programID = id;
}

std::string Shader::ParseShader(const std::string& filepath) {
	std::ifstream stream(filepath);

	std::stringstream ss;
	std::string line;
	ShaderType type = ShaderType::NONE;
	while (getline(stream, line))
	{
		ss << line << "\n";
	}
	return ss.str();
}

void Shader::Init() {
	GLCall(unsigned int tprogramID = glCreateProgram());

	CompileShader(GL_VERTEX_SHADER, ParseShader(paths[0]), m_vert_shader_id);
	CompileShader(GL_FRAGMENT_SHADER, ParseShader(paths[1]), m_frag_shader_id);

	UseShader(m_vert_shader_id, tprogramID);
	UseShader(m_frag_shader_id, tprogramID);

	SetProgramID(tprogramID);

	InitUniforms();
}

void Shader::ActivateProgram() {
	GLCall(glLinkProgram(GetProgramID()));
	GLCall(glValidateProgram(GetProgramID()));
	GLCall(glUseProgram(GetProgramID()));

	GLCall(glDeleteShader(m_vert_shader_id));
	GLCall(glDeleteShader(m_frag_shader_id));
}

unsigned int Shader::GetUniform(const std::string& name) {
	GLCall(int location = glGetUniformLocation(GetProgramID(), name.c_str()));
	if (location == -1) {
		PrintUtils::PrintError(std::format("ERROR: COULD NOT FIND UNIFORM '{}'", name));
		exit(1);
	}
	return location;
}


void Shader::CompileShader(unsigned int type, const std::string& source, unsigned int& shaderID) {
	GLCall(shaderID = glCreateShader(type));
	const char* src = source.c_str();
	GLCall(glShaderSource(shaderID, 1, &src, nullptr));
	GLCall(glCompileShader(shaderID));

	int result;
	GLCall(glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result));

	if (result == GL_FALSE) {
		int length;
		GLCall(glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &length));
		char* message = (char*)alloca(length * sizeof(char));
		GLCall(glGetShaderInfoLog(shaderID, length, &length, message));

		PrintUtils::PrintError(std::format("FAILED TO COMPILE {} SHADER: {}", type == GL_VERTEX_SHADER ? "vertex" : "fragment", message));
	}
}


void Shader::UseShader(unsigned int& id, unsigned int& program) {
	GLCall(glAttachShader(program, id));
	GLCall(glDeleteShader(id));
}
