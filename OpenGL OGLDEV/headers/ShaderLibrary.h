#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <sstream>

constexpr unsigned int basicProgramIndex = 0;
constexpr unsigned int testProgramIndex = 1;
constexpr unsigned int numShaders = 2;

class ShaderLibrary {
public:
	ShaderLibrary() = default;

	void Init();

	enum class ShaderType {
		NONE = -1, VERTEX = 0, FRAGMENT = 1
	};

	void UseShader(unsigned int& id, unsigned int& program);
	void ActivateProgram(unsigned int& program);

	std::vector<unsigned int> programIDs{ numShaders };

	unsigned int activeProgramIndex;

private:
	std::string ParseShader(const std::string& filepath);
	unsigned int CreateShader(const std::string& shaderSource, ShaderType shader_type);
	void CompileShader(unsigned int type, const std::string& source, unsigned int& id);

	unsigned int basicVertShaderID;
	unsigned int basicFragShaderID;

	unsigned int testFragShaderID;
	unsigned int testVertShaderID;


};