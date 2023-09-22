#include "pch/pch.h"

#include "util/util.h"
#include "shaders/Shader.h"
#include "util/Log.h"


namespace ORNG {

	Shader::~Shader() {
		glDeleteProgram(m_program_id);
	}



	std::string Shader::ParseShader(const std::string& filepath, const std::vector<std::string>& defines) {
		std::ifstream stream(filepath);

		std::stringstream ss;
		std::string line;
		// Output version statement first
		getline(stream, line);
		ss << line << "\n";
		for (auto& define : defines) { // insert definitions 
			ss << "#define" << " " << define << "\n";
		}
		while (getline(stream, line))
		{
			ss << line << "\n";
		}
		return ss.str();
	}

	void Shader::AddStage(GLenum shader_type, const std::string& filepath, const std::vector<std::string>& defines) {
		ASSERT(std::filesystem::exists(filepath));
		unsigned int shader_handle = 0;
		CompileShader(shader_type, ParseShader(filepath, defines), shader_handle);
		m_shader_handles.push_back(shader_handle);
	}

	void Shader::AddStageFromString(GLenum shader_type, const std::string& shader_code, const std::vector<std::string>& defines) {
		unsigned int shader_handle = 0;
		std::string shader_code_copy = shader_code;

		for (auto& define : defines) { // insert definitions 
			shader_code_copy.insert(shader_code_copy.find("core") + 4, "\n" "#define " + define + "\n");
		}

		CompileShader(shader_type, shader_code_copy, shader_handle);
		m_shader_handles.push_back(shader_handle);
	}

	void Shader::Init() {
		unsigned int tprogramID = glCreateProgram();

		for (unsigned int handle : m_shader_handles) {
			UseShader(handle, tprogramID);
		}

		m_program_id = tprogramID;

		glLinkProgram(m_program_id);

		int result;
		glGetProgramiv(m_program_id, GL_LINK_STATUS, &result);

		if (result == GL_FALSE) {
			int length;
			glGetProgramiv(m_program_id, GL_INFO_LOG_LENGTH, &length);
			char* message = (char*)alloca(length * sizeof(char));
			glGetProgramInfoLog(m_program_id, length, &length, message);


			ORNG_CORE_CRITICAL("Failed to link program for shader '{0}' : '{1}", m_name, message);
			BREAKPOINT;
		}

		glValidateProgram(m_program_id);
		glUseProgram(m_program_id);
	}


	unsigned int Shader::CreateUniform(const std::string& name) {
		int location = glGetUniformLocation(m_program_id, name.c_str());
		if (location == -1 && SHADER_DEBUG_MODE == true) {
			ORNG_CORE_ERROR("Could not find uniform '{0}'", name);
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

			std::string shader_type_name;

			switch (type) {
			case GL_VERTEX_SHADER:
				shader_type_name = "vertex";
				break;
			case GL_FRAGMENT_SHADER:
				shader_type_name = "fragment";
				break;
			case GL_COMPUTE_SHADER:
				shader_type_name = "compute";
				break;
			}

			ORNG_CORE_CRITICAL("Failed to compile {0} shader '{1}': {2}", shader_type_name, m_name, message);
		}
	}


	void Shader::UseShader(unsigned int& id, unsigned int program) {
		glAttachShader(program, id);
		glDeleteShader(id);
	}
}