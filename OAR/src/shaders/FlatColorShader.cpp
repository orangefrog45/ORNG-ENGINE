#include <shaders/FlatColourShader.h>
#include <glew.h>
#include <glfw/glfw3.h>

void FlatColorShader::InitUniforms() {
	ActivateProgram();
	m_color_loc = GetUniform("color");
}

void FlatColorShader::SetColor(float r, float g, float b) {
	glUniform3f(m_color_loc, r, g, b);
}