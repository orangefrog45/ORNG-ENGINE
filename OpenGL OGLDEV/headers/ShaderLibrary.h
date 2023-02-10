#pragma once
#include <string>
#include <fstream>
#include <sstream>
class ShaderLibrary {
public:
	ShaderLibrary() = default;

	void Init();

	enum class ShaderType {
		NONE = -1, VERTEX = 0, FRAGMENT = 1
	};

	void UseShader(unsigned int& id);
	void ActivateProgram();
	unsigned int program;
	unsigned int basicVertShaderID;
	unsigned int basicFragShaderID;

private:
	std::string ParseShader(const std::string& filepath);
	unsigned int CreateShader(const std::string& shaderSource, ShaderType shader_type);
	void CompileShader(unsigned int type, const std::string& source, unsigned int& id);


};