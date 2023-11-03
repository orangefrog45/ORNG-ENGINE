#include "pch/pch.h"

#include "util/util.h"
#include "shaders/Shader.h"
#include "util/Log.h"


namespace ORNG {
	Shader::~Shader() {
		glDeleteProgram(m_program_id);
	}
	void CheckIncludeTreeCircularIncludes(const std::string& filepath, std::vector<const std::string*>& include_tree) {
		for (const auto* s : include_tree) {
			if (*s == filepath) {
				ORNG_CORE_ERROR("Circular include detected between shader files, include tree:");
				for (auto* s1 : include_tree) {
					ORNG_CORE_ERROR(*s1);
				}
				BREAKPOINT;
			}
		}
	}

	bool HeaderGuardTriggered(std::vector<std::string>& defines, const std::string& filepath) {
		std::string define = filepath;
		define.erase(std::remove_if(define.begin(), define.end(), [](char c) { return !std::isalnum(c); }), define.end());
		// Macro can't start with digits
		while (std::isdigit(define[0])) {
			define.erase(0);
		}

		if (std::ranges::find(defines, define) != defines.end()) {
			return true;
		}

		// Add "header guard"
		defines.push_back(define);

		return false;
	}

	void Shader::ParseShaderInclude(const std::string& filepath, std::vector<std::string>& defines, std::stringstream& stream, std::vector<const std::string*>& include_tree) {
		CheckIncludeTreeCircularIncludes(filepath, include_tree);
		include_tree.push_back(&filepath);

		std::ifstream ifstream(filepath);
		std::string line;

		// Automatic header guard
		if (HeaderGuardTriggered(defines, filepath)) {
			include_tree.pop_back();
			return;
		}


		std::string include_search_directory = GetFileDirectory(filepath) + "\\";
		while (getline(ifstream, line))
		{
			if (line.find("ORNG_INCLUDE") != std::string::npos) {
				size_t first = line.find("\"") + 1;
				size_t last = line.rfind("\"");
				std::string include_fp = include_search_directory + line.substr(first, last - first);
				if (FileExists(include_fp))
					ParseShaderInclude(include_fp, defines, stream, include_tree);
				else
					ORNG_CORE_ERROR("Shader '{0}' include directive for file '{1}' failed, file not found", m_name, include_fp);
			}
			else {
				stream << line << "\n";
			}
		}

		include_tree.pop_back();
	}

	void Shader::ParseShaderIncludeString(const std::string& filepath, std::vector<std::string>& defines, std::string& str, size_t directive_pos, std::vector<const std::string*>& include_tree) {
		std::string s = ParseShader(filepath, defines, include_tree);
		str.insert(directive_pos, s);
		s += "\n";
	}


	std::string Shader::ParseShader(const std::string& filepath, std::vector<std::string>& defines, std::vector<const std::string*>& include_tree) {
		CheckIncludeTreeCircularIncludes(filepath, include_tree);
		include_tree.push_back(&filepath);

		// Automatic header guard
		if (HeaderGuardTriggered(defines, filepath)) {
			include_tree.pop_back();
			return "";
		}




		std::ifstream stream(filepath);
		std::string line;
		std::stringstream ss;

		// Output version statement first
		getline(stream, line);

		if (line.starts_with("#version"))
			ss << line << "\n";

		for (int i = 0; i < defines.size(); i++) { // insert definitions
			const std::string& define = defines[i];
			if (std::ranges::count(defines, define) > 1)
				continue;

			ss << "#define" << " " << define << "\n";
			// Make a copy that I can track, if there is a copy of a define I don't need to include it, automatic header guard
			defines.push_back(define);
		}

		if (!line.starts_with("#version"))
			goto loop;


		while (getline(stream, line))
		{
		loop:
			if (line.find("ORNG_INCLUDE") != std::string::npos) {
				size_t first = line.find("\"") + 1;
				size_t last = line.rfind("\"");

				std::string include_fp = line.substr(first, last - first);
				std::string shader_dir = GetFileDirectory(filepath) + "\\";
				if (FileExists(shader_dir + include_fp))
					ParseShaderInclude(shader_dir + include_fp, defines, ss, include_tree);
				else
					ORNG_CORE_ERROR("Shader '{0}' include directive for file '{1}' failed, file not found", m_name, include_fp);
			}
			else {
				ss << line << "\n";
			}
		}
		include_tree.pop_back();
		return ss.str();
	}

	void Shader::AddStage(GLenum shader_type, const std::string& filepath, std::vector<std::string> defines) {
		ASSERT(FileExists(filepath));
		unsigned int shader_handle = 0;
		std::vector<const std::string*> include_tree;
		CompileShader(shader_type, ParseShader(filepath, defines, include_tree), shader_handle);
		m_shader_handles.push_back(shader_handle);
	}

	void Shader::AddStageFromString(GLenum shader_type, const std::string& shader_code, std::vector<std::string> defines) {
		unsigned int shader_handle = 0;
		std::string shader_code_copy = shader_code;

		auto define_insert_pos = shader_code_copy.find("core") + 4;

		if (define_insert_pos == std::string::npos)
			define_insert_pos = 0;

		for (int i = 0; i < defines.size(); i++) { // insert definitions
			const std::string& define = defines[i];

			if (std::ranges::count(defines, define) > 1)
				continue;


			shader_code_copy.insert(define_insert_pos, "\n" "#define " + define + "\n");
			defines.push_back(define);
		}

		// Handle include directives
		size_t pos = shader_code_copy.find("ORNG_INCLUDE");
		// Copy needs to be made so the includes can be written to it
		while (pos != std::string::npos) {
			size_t first = shader_code_copy.find("\"", pos) + 1;
			size_t last = shader_code_copy.find("\"", first) + 1;
			std::string include_fp = shader_code_copy.substr(first, last - first - 1);
			// All string shaders are located in here so searches will be relative to this
			std::string shader_include_dir = ORNG_CORE_MAIN_DIR "\\res\\shaders\\";

			shader_code_copy.erase(pos, last - pos);
			std::vector<const std::string*> include_tree;
			if (FileExists(shader_include_dir + include_fp)) {
				ParseShaderIncludeString(shader_include_dir + include_fp, defines, shader_code_copy, pos, include_tree);
			}
			else
				ORNG_CORE_ERROR("Shader '{0}' include directive for file '{1}' failed, file not found", m_name, include_fp);

			pos = shader_code_copy.find("ORNG_INCLUDE", pos - (last - pos));
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