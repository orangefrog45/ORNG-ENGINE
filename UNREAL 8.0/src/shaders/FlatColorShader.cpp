#include <shaders/FlatColourShader.h>

void FlatColorShader::InitUniforms() {
	ActivateProgram();
	m_world_transform_loc = GetUniform("world_transform");
	m_color_loc = GetUniform("color");
}