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

ShaderLibrary::ShaderLibrary() {
	paths = { "res/shaders/SkyboxVS.shader",
		"res/shaders/SkyboxFS.shader",
		"res/shaders/BasicVS.shader",
		"res/shaders/BasicFS.shader",
		"res/shaders/TestVS.shader",
		"res/shaders/TestFS.shader" };

}

void ShaderLibrary::Init() {
	for (unsigned int i = 0; i < paths.size(); i += 2) {
		unsigned int vertID;
		unsigned int fragID;
		GLCall(unsigned int programID = glCreateProgram());

		CompileShader(GL_VERTEX_SHADER, ParseShader(paths[i]), vertID);
		CompileShader(GL_FRAGMENT_SHADER, ParseShader(paths[i + 1]), fragID);

		UseShader(vertID, programID);
		UseShader(fragID, programID);

		programIDs.push_back(programID);
	}

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