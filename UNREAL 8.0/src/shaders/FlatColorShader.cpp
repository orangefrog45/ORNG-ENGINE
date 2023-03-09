#include <shaders/FlatColourShader.h>

void FlatColorShader::Init() {
	GLCall(unsigned int tprogramID = glCreateProgram());

	CompileShader(GL_VERTEX_SHADER, ParseShader("res/shaders/FlatColourVS.shader"), m_vert_shader_id);
	CompileShader(GL_FRAGMENT_SHADER, ParseShader("res/shaders/FlatColourFS.shader"), m_frag_shader_id);

	UseShader(m_vert_shader_id, tprogramID);
	UseShader(m_frag_shader_id, tprogramID);

	SetProgramID(tprogramID);

	ActivateProgram();
	InitUniforms();
}

void FlatColorShader::ActivateProgram() {
	GLCall(glLinkProgram(GetProgramID()));
	GLCall(glValidateProgram(GetProgramID()));
	GLCall(glUseProgram(GetProgramID()));

	GLCall(glDeleteShader(m_vert_shader_id));
	GLCall(glDeleteShader(m_frag_shader_id));

}

void FlatColorShader::InitUniforms() {
	m_world_transform_loc = GetUniform("world_transform");
	m_color_loc = GetUniform("color");
}