#pragma once
#include "shaders/LightingShader.h"

class ShaderLibrary {
public:
	friend class Renderer;
	ShaderLibrary() {};
	void Init();

	/* Give shader filepaths in order: Vertex, Fragment */
	Shader& CreateShader(const char* name, const std::array<const char*, 2>& shader_filepaths);
	void SetMatrixUBOs(glm::mat4& proj, glm::mat4& view);

	Shader& GetShader(const char* name);
	void DeleteShader(const char* name);

	LightingShader lighting_shader;
private:
	[[nodiscard]] unsigned int CreateShaderID() { return m_last_id++; };

	std::unordered_map<std::string, Shader> m_shaders;
	unsigned int m_last_id = 1;
	unsigned int m_matrix_UBO;
};