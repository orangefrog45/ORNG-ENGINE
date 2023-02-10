#include <ShaderLibrary.h>
#include <string>
#include <sstream>
#include <glew.h>
#include <iostream>
#include <freeglut.h>
#include <fstream>
#include "GLErrorHandling.h"
#include "util.h"

void ShaderLibrary::Init() {
	GLCall(program = glCreateProgram());

	CompileShader(GL_VERTEX_SHADER, ParseShader("res/shaders/Basic.vs"), basicVertShaderID);
	CompileShader(GL_FRAGMENT_SHADER, ParseShader("res/shaders/Basic.fs"), basicFragShaderID);

}


std::string ShaderLibrary::ParseShader(const std::string& filepath) {
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


void ShaderLibrary::CompileShader(unsigned int type, const std::string& source, unsigned int& id) {
	GLCall(id = glCreateShader(type));
	const char* src = source.c_str();
	GLCall(glShaderSource(id, 1, &src, nullptr));
	GLCall(glCompileShader(id));

	int result;
	GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &result));

	if (result == GL_FALSE) {
		int length;
		GLCall(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));
		char* message = (char*)alloca(length * sizeof(char));
		GLCall(glGetShaderInfoLog(id, length, &length, message));

		std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!" << std::endl;
		std::cout << message << std::endl;

	}
}

/*unsigned int ShaderLibrary::CreateShader(const std::string& shaderSource, ShaderType shader_type) {
	unsigned int shaderid;

	switch (shader_type) {
	case(ShaderType::VERTEX):
		shaderid = CompileShader(GL_VERTEX_SHADER, shaderSource);
		break;

	case(ShaderType::FRAGMENT):
		shaderid = CompileShader(GL_FRAGMENT_SHADER, shaderSource);
		break;
	}

	return shaderid;


}*/
void ShaderLibrary::UseShader(unsigned int& id) {
	GLCall(glAttachShader(program, id));
	GLCall(glDeleteShader(id));

}

void ShaderLibrary::ActivateProgram() {
	GLCall(glLinkProgram(program));
	GLCall(glValidateProgram(program));
	GLCall(glUseProgram(program));
}