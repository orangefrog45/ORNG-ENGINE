#include <iostream>
#include "Shader.h"
#include "glew.h"
#include "util.h"


unsigned int Shader::Init() {
	unsigned int vertID;
	unsigned int fragID;
	GLCall(unsigned int programID = glCreateProgram());

	CompileShader(GL_VERTEX_SHADER, ParseShader(vertexShaderPath), vertID);
	CompileShader(GL_FRAGMENT_SHADER, ParseShader(fragmentShaderPath), fragID);

	UseShader(vertID, programID);
	UseShader(fragID, programID);

	m_programID = programID;
	return programID;
}

unsigned int Shader::GetProgramID() {
	return m_programID;
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

		std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!" << std::endl;
		std::cout << message << std::endl;

	}
}


void Shader::UseShader(unsigned int& id, unsigned int& program) {
	GLCall(glAttachShader(program, id));
	GLCall(glDeleteShader(id));
}

