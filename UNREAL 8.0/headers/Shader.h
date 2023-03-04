#pragma once
#include <glew.h>
#include <vector>
#include <string>
#include "util/util.h"

class Shader {
public:

	~Shader();

	enum class ShaderType {
		NONE = -1, VERTEX = 0, FRAGMENT = 1
	};

	virtual void Init() = 0;

	virtual void ActivateProgram() = 0;

	const GLint GetProgramID();

protected:
	unsigned int GetUniform(const std::string& name);

	virtual void InitUniforms() = 0;

	void CompileShader(unsigned int type, const std::string& source, unsigned int& shaderID);

	void UseShader(unsigned int& id, unsigned int& program);

	std::string ParseShader(const std::string& filepath);

	void SetProgramID(const GLint);


private:
	unsigned int m_programID;
};