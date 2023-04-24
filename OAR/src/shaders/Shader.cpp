#include "pch/pch.h"

#include "util/util.h"
#include "shaders/Shader.h"
#include "util/Log.h"


const unsigned int Shader::GetProgramID() {
	return m_programID;
}

Shader::~Shader() {
	GLCall(glDeleteProgram(GetProgramID()));
}

void Shader::SetProgramID(const unsigned int id) {
	m_programID = id;
}

std::string Shader::ParseShader(const std::string& filepath) {
	std::ifstream stream(filepath);

	std::stringstream ss;
	std::string line;
	ShaderType type = ShaderType::NONE;
	while (getline(stream, line))
	{
		ss << line << "\n";
	}
	return ss.str();
}

void Shader::AddUBO(const std::string& name, unsigned int storage_size, int draw_type, unsigned int buffer_base) {
	m_uniforms[name] = 0;
	const unsigned int* ubo = &(m_uniforms.at(name));

	GLCall(glGenBuffers(1, &m_uniforms[name]));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, *ubo));
	GLCall(glBufferData(GL_UNIFORM_BUFFER, storage_size, nullptr, draw_type));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
	GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, buffer_base, *ubo));
}

void Shader::Init() {
	GLCall(unsigned int tprogramID = glCreateProgram());

	CompileShader(GL_VERTEX_SHADER, ParseShader(paths[0]), m_vert_shader_id);
	CompileShader(GL_FRAGMENT_SHADER, ParseShader(paths[1]), m_frag_shader_id);

	UseShader(m_vert_shader_id, tprogramID);
	UseShader(m_frag_shader_id, tprogramID);

	SetProgramID(tprogramID);
}

void Shader::ActivateProgram() {
	GLCall(glLinkProgram(GetProgramID()));
	GLCall(glValidateProgram(GetProgramID()));
	GLCall(glUseProgram(GetProgramID()));

	GLCall(glDeleteShader(m_vert_shader_id));
	GLCall(glDeleteShader(m_frag_shader_id));
}

unsigned int Shader::CreateUniform(const std::string& name) {
	GLCall(int location = glGetUniformLocation(GetProgramID(), name.c_str()));
	if (location == -1 && SHADER_DEBUG_MODE == true) {
		OAR_CORE_ERROR("Could not find uniform '{0}'", name);
		BREAKPOINT;
	}
	return location;
}


void Shader::CompileShader(unsigned int type, const std::string& source, unsigned int& shaderID) {
	GLCall(shaderID = glCreateShader(type));
	const char* src = source.c_str();
	GLCall(glShaderSource(shaderID, 1, &src, nullptr));
	GLCall(glCompileShader(shaderID));

	int result;
	GLCall(glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result));

	if (result == GL_FALSE) {
		int length;
		GLCall(glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &length));
		char* message = (char*)alloca(length * sizeof(char));
		GLCall(glGetShaderInfoLog(shaderID, length, &length, message));

		OAR_CORE_CRITICAL("Failed to compile {0} shader: {1}", type == GL_VERTEX_SHADER ? "vertex" : "fragment", message);
	}
}


void Shader::UseShader(unsigned int& id, unsigned int& program) {
	GLCall(glAttachShader(program, id));
	GLCall(glDeleteShader(id));
}
