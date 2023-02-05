#include <string>
#include <sstream>
#include <glew.h>
#include <iostream>
#include <freeglut.h>
#include <fstream>
#include "shaderhandling.h"

#define ASSERT(x) if (!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
    x;\
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))



static void GLClearError() {
	while (glGetError() != GL_NO_ERROR);
}

static bool GLLogCall(const char* function, const char* file, int line) {
	while (GLenum error = glGetError()) {
		std::cout << "[OpenGL Error] (" << error << ")" << " " << function << " : " << file << " : " << line << std::endl;
		return false;
	}
	return true;
}


 ShaderProgramSource ParseShader(const std::string& filepath) {
	std::ifstream stream(filepath);

	enum class ShaderType {
		NONE = -1, VERTEX = 0, FRAGMENT = 1
	};

	std::stringstream ss[2];
	std::string line;
	ShaderType type = ShaderType::NONE;
	while (getline(stream, line))
	{
		if (line.find("#shader") != std::string::npos)
		{
			if (line.find("vertex") != std::string::npos) {
				type = ShaderType::VERTEX;
			}
			else if (line.find("fragment") != std::string::npos) {
				type = ShaderType::FRAGMENT;
			}
		}
		else {
			ss[(int)type] << line << "\n";
		}

	}
	return { ss[0].str(), ss[1].str() };
}

unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader) {
	 GLCall(unsigned int program = glCreateProgram());
	 unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
	 unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

	 GLCall(glAttachShader(program, vs));
	 GLCall(glAttachShader(program, fs));
	 GLCall(glLinkProgram(program));
	 GLCall(glValidateProgram(program));

	 GLCall(glDeleteShader(vs));
	 GLCall(glDeleteShader(fs));

	 return program;


 }

unsigned int CompileShader(unsigned int type, const std::string& source) {
	GLCall(unsigned int id = glCreateShader(type));
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

		GLCall(glDeleteShader(id));
		return 0;

	}

	return id;
}

void AttachShader(const std::string filePath) {
	ShaderProgramSource source = ParseShader(filePath);
	unsigned int shader = CreateShader(source.vertexSource, source.fragmentSource);
	GLCall(glUseProgram(shader));
}