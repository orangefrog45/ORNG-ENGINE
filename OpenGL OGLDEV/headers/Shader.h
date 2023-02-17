#pragma once
#include <glew.h>
#include <vector>
#include <string>
#include "util.h"

class Shader {
public:

	enum class ShaderType {
		NONE = -1, VERTEX = 0, FRAGMENT = 1
	};

	void ActivateProgram();


	void CompileShader(unsigned int type, const std::string& source, unsigned int& shaderID);

	void UseShader(unsigned int& id, unsigned int& program);

	std::string ParseShader(const std::string& filepath);

	const GLint& GetProgramID();

	void SetProgramID(const GLint);

private:
	virtual void InitUniforms() {};
	unsigned int programID;
};