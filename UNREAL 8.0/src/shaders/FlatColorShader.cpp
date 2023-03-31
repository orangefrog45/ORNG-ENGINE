#include <shaders/FlatColourShader.h>

void FlatColorShader::InitUniforms() {
	ActivateProgram();
	m_color_loc = GetUniform("color");
}