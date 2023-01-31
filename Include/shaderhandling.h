#pragma once

struct ShaderProgramSource
{
	std::string vertexSource;
	std::string fragmentSource;
};

ShaderProgramSource ParseShader(const std::string& filepath);

unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader);

unsigned int CompileShader(unsigned int type, const std::string& source);