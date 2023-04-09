#pragma once
#include <vector>
#include <string>

class Material;

class Shader {
public:

	~Shader();

	enum class ShaderType {
		NONE = -1, VERTEX = 0, FRAGMENT = 1
	};

	virtual void Init();

	virtual void ActivateProgram();

	const unsigned int GetProgramID();

	virtual void SetMaterial(const Material& material) {};//not all shaders need materials e.g flat colour shader

protected:
	unsigned int GetUniform(const std::string& name);

	virtual void InitUniforms() = 0;

	void CompileShader(unsigned int type, const std::string& source, unsigned int& shaderID);

	void UseShader(unsigned int& id, unsigned int& program);

	std::string ParseShader(const std::string& filepath);

	void SetProgramID(const unsigned int);

	unsigned int m_vert_shader_id;
	unsigned int m_frag_shader_id;
	unsigned int m_programID;

	std::vector<std::string> paths;

};