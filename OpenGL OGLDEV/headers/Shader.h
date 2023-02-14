#pragma once
#include <string>
#include "ShaderLibrary.h"

class Shader {
public:
	unsigned int Init();
	virtual void InitUniforms();
	unsigned int GetProgramID();
private:
	enum class ShaderType {
		NONE = -1, VERTEX = 0, FRAGMENT = 1
	};

	std::string ParseShader(const std::string& filepath);
	void CompileShader(unsigned int type, const std::string& source, unsigned int& shaderID);
	void UseShader(unsigned int& id, unsigned int& program);
	unsigned int WVPLocation;
	unsigned int m_programID;
	std::string vertexShaderPath;
	std::string fragmentShaderPath;

};
