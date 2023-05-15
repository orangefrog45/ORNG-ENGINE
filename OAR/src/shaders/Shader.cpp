#include "pch/pch.h"

#include "util/util.h"
#include "shaders/Shader.h"
#include "util/Log.h"


namespace ORNG {

	Shader::~Shader() {
		glDeleteProgram(m_program_id);
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

		glGenBuffers(1, &m_uniforms[name]);
		glBindBuffer(GL_UNIFORM_BUFFER, *ubo);
		glBufferData(GL_UNIFORM_BUFFER, storage_size, nullptr, draw_type);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		glBindBufferBase(GL_UNIFORM_BUFFER, buffer_base, *ubo);
	}

	void Shader::AddStage(GLenum shader_type, const std::string& filepath) {
		unsigned int shader_handle = 0;
		CompileShader(shader_type, ParseShader(filepath), shader_handle);
		m_shader_handles.push_back(shader_handle);
	}

	void Shader::Init() {
		unsigned int tprogramID = glCreateProgram();

		for (unsigned int handle : m_shader_handles) {
			UseShader(handle, tprogramID);
		}

		m_program_id = tprogramID;

		glLinkProgram(m_program_id);
		glValidateProgram(m_program_id);
		glUseProgram(m_program_id);
	}


	unsigned int Shader::CreateUniform(const std::string& name) {
		int location = glGetUniformLocation(m_program_id, name.c_str());
		if (location == -1 && SHADER_DEBUG_MODE == true) {
			OAR_CORE_ERROR("Could not find uniform '{0}'", name);
			BREAKPOINT;
		}
		return location;
	}


	void Shader::CompileShader(unsigned int type, const std::string& source, unsigned int& shaderID) {
		shaderID = glCreateShader(type);
		const char* src = source.c_str();
		glShaderSource(shaderID, 1, &src, nullptr);
		glCompileShader(shaderID);

		int result;
		glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);

		if (result == GL_FALSE) {
			int length;
			glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &length);
			char* message = (char*)alloca(length * sizeof(char));
			glGetShaderInfoLog(shaderID, length, &length, message);

			OAR_CORE_CRITICAL("Failed to compile {0} shader: {1}", type == GL_VERTEX_SHADER ? "vertex" : "fragment", message);
		}
	}


	void Shader::UseShader(unsigned int& id, unsigned int& program) {
		glAttachShader(program, id);
		glDeleteShader(id);
	}
}