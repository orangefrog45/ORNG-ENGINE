#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <glew.h>
#include <sstream>
#include "util.h"

constexpr unsigned int skyboxProgramIndex = 0;
constexpr unsigned int basicProgramIndex = 1;
constexpr unsigned int testProgramIndex = 2;
constexpr unsigned int numShaders = 3;

struct Shader {
	Shader() : WVPLocation(0), samplerLocation(0), programID(0) {};
	Shader(GLuint WVP, GLuint samp, GLuint programID) : WVPLocation(WVP), samplerLocation(samp), programID(programID) {};
	GLint WVPLocation;
	GLint samplerLocation;
	GLuint programID;

	void ActivateProgram() {
		GLCall(glLinkProgram(programID));
		GLCall(glValidateProgram(programID));
		GLCall(glUseProgram(programID));

		if (!WVPLocation) {
			GLCall(WVPLocation = glGetUniformLocation(programID, "gTransform"));
			ASSERT(WVPLocation != -1);
			GLCall(samplerLocation = glGetUniformLocation(programID, "gSampler"));
			ASSERT(samplerLocation != -1);
		}

	}
};

class ShaderLibrary {
public:
	ShaderLibrary();

	void Init();

	enum class ShaderType {
		NONE = -1, VERTEX = 0, FRAGMENT = 1
	};

	void UseShader(unsigned int& id, unsigned int& program);

	std::vector<std::string> paths;

	std::vector<Shader> shaderData;


private:
	std::string ParseShader(const std::string& filepath);
	void CompileShader(unsigned int type, const std::string& source, unsigned int& id);

};