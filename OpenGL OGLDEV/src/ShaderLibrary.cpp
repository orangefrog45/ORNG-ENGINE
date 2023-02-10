#include <ShaderLibrary.h>
#include <string>
#include <sstream>
#include <glew.h>
#include <iostream>
#include <freeglut.h>
#include <fstream>
#include <vector>
#include <algorithm>
#include "GLErrorHandling.h"
#include "util.h"

void ShaderLibrary::Init() {
	programIDs = { 0, 0 };
	unsigned int& basicProgram = programIDs[basicProgramIndex];
	unsigned int& testProgram = programIDs[testProgramIndex];
	GLCall(basicProgram = glCreateProgram());
	GLCall(testProgram = glCreateProgram());


	CompileShader(GL_VERTEX_SHADER, ParseShader("res/shaders/BasicVS.shader"), basicVertShaderID);
	CompileShader(GL_FRAGMENT_SHADER, ParseShader("res/shaders/BasicFS.shader"), basicFragShaderID);
	UseShader(basicVertShaderID, basicProgram);
	UseShader(basicFragShaderID, basicProgram);


	CompileShader(GL_VERTEX_SHADER, ParseShader("res/shaders/TestVS.shader"), testVertShaderID);
	CompileShader(GL_FRAGMENT_SHADER, ParseShader("res/shaders/TestFS.shader"), testFragShaderID);
	UseShader(testVertShaderID, testProgram);
	UseShader(testFragShaderID, testProgram);
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


void ShaderLibrary::CompileShader(unsigned int type, const std::string& source, unsigned int& shaderID) {
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
void ShaderLibrary::UseShader(unsigned int& id, unsigned int& program) {
	GLCall(glAttachShader(program, id));
	GLCall(glDeleteShader(id));
}

void ShaderLibrary::ActivateProgram(unsigned int& program) {
	GLCall(glLinkProgram(program));
	GLCall(glValidateProgram(program));
	GLCall(glUseProgram(program));

	activeProgramIndex = std::find(programIDs.begin(), programIDs.end(), program) - programIDs.begin();
}