#include "pch/pch.h"

#include "util/util.h"
#include "shaders/Shader.h"
#include "util/Log.h"
#include <regex>

#include "rendering/Renderer.h"

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


	std::string FindShaderIncludePath(const std::string& filepath) {
		std::string file_directory = GetFileDirectory(filepath) + "\\";
		std::string filename = filepath.substr(file_directory.size());

		std::array<std::string, 3> include_directories = {
			file_directory,
			"res\\core-res\\shaders\\",
			"res\\shaders\\",
		};

		for (size_t i = 0; i < include_directories.size(); i++) {
			if (auto found_filepath = include_directories[i] + filename; FileExists(found_filepath)) {
				return found_filepath;
			}
		}

		return "";
	}

	void Shader::ParseShaderInclude(const std::string& filepath, std::vector<std::string>& defines, std::stringstream& stream, std::vector<const std::string*>& include_tree, unsigned& line_count,
		GLenum shader_type) {
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

				if (std::string inc_fp = FindShaderIncludePath(include_fp); !inc_fp.empty()) {
					unsigned before_include_line_count = line_count;
					ParseShaderInclude(inc_fp, defines, stream, include_tree, line_count, shader_type);
				}
				else
					ORNG_CORE_ERROR("Shader include directive for file '{0}' failed, file not found", include_fp);
			}
			else {
				stream << line << "\n";
				line_count++;
			}
		}


		include_tree.pop_back();
	}

	void Shader::ParseShaderIncludeString(const std::string& filepath, std::vector<std::string>& defines, std::string& shader_str, size_t directive_pos, std::vector<const std::string*>& include_tree, unsigned& line_count, GLenum shader_type) {
		auto result = ParseShader(filepath, defines, include_tree, line_count, shader_type);

		line_count += result.line_count;

		shader_str.insert(directive_pos, result.shader_code);
	}


	Shader::ParsedShaderData Shader::ParseShader(const std::string& filepath, std::vector<std::string>& defines, std::vector<const std::string*>& include_tree, unsigned line_count, GLenum shader_type) {
		CheckIncludeTreeCircularIncludes(filepath, include_tree);
		include_tree.push_back(&filepath);

		// Automatic header guard
		if (HeaderGuardTriggered(defines, filepath)) {
			include_tree.pop_back();
			return { "", 0 };
		}


		std::ifstream stream(filepath);
		std::string line;
		std::stringstream ss;

		// Output version statement first
		getline(stream, line);

		if (line.starts_with("#version"))
			ss << line << "\n";

		if (line_count == 0)
			ss << "#line 1" << "\n";

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

				std::string include_filename = line.substr(first, last - first);
				std::string shader_dir = GetFileDirectory(filepath) + "\\";

				if (std::string inc_fp = FindShaderIncludePath(shader_dir + include_filename); !inc_fp.empty())
					ParseShaderInclude(inc_fp, defines, ss, include_tree, line_count, shader_type);
				else
					ORNG_CORE_ERROR("Shader include directive for file '{0}' failed, file not found", include_filename);
			}
			else {
				ss << line << "\n";
				line_count++;
			}
		}
		include_tree.pop_back();
		return ParsedShaderData{ std::move(ss.str()), line_count };
	}

	void Shader::AddStage(GLenum shader_type, const std::string& filepath, std::vector<std::string> defines) {
		unsigned int shader_handle = 0;

		if (Renderer::GetShaderLibrary().ShaderPackageIsLoaded()) {
			// Shaders will be deserialized and loaded from a shader package
			ShaderData data{ .name = m_name, .stage = (uint32_t)shader_type, .id = 0 };
			CompileShader(shader_type, Renderer::GetShaderLibrary().PopShaderCodeFromCache(data), shader_handle);
			m_shader_handles.push_back(shader_handle);
		}
		else {
			ASSERT(FileExists(filepath));
			std::vector<const std::string*> include_tree;

			m_stages[shader_type] = { std::filesystem::absolute(filepath).string(), defines }; // Store absolute path as the working directory will change to fit the project
			auto result = ParseShader(filepath, defines, include_tree, 0, shader_type);
			CompileShader(shader_type, result.shader_code, shader_handle);

			m_shader_handles.push_back(shader_handle);
		}
	}

	void Shader::AddStageFromString(GLenum shader_type, const std::string& shader_code, std::vector<std::string> defines) {
		if (Renderer::GetShaderLibrary().ShaderPackageIsLoaded())
			return; // Shaders will be deserialized and loaded from a shader package, so this behaviour can be scrapped

		// Copy needs to be made so the includes can be written to it
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

		// All string shaders are located in here so searches will be relative to this
		const char* shader_include_dir = ORNG_CORE_MAIN_DIR "\\res\\shaders\\";

		unsigned line_number = 0;
		// Handle include directives
		size_t pos = shader_code_copy.find("ORNG_INCLUDE");
		while (pos != std::string::npos) {
			size_t first = shader_code_copy.find("\"", pos) + 1;
			size_t last = shader_code_copy.find("\"", first) + 1;
			shader_code_copy.erase(pos, last - pos);
			std::string include_fp = shader_code_copy.substr(first, last - first - 1);

			std::vector<const std::string*> include_tree;
			if (std::string full_include_fp = shader_include_dir + include_fp; FileExists(full_include_fp))
				ParseShaderIncludeString(full_include_fp, defines, shader_code_copy, pos, include_tree, line_number, shader_type);
			else
				ORNG_CORE_ERROR("Shader '{0}' include directive for file '{1}' failed, file not found", m_name, include_fp);

			pos = shader_code_copy.find("ORNG_INCLUDE", pos - (last - pos));
		}

		unsigned int shader_handle = 0;
		CompileShader(shader_type, shader_code_copy, shader_handle);
		m_shader_handles.push_back(shader_handle);
	}

	void Shader::Reload() {
		glDeleteProgram(m_program_id);

		for (auto& [type, stage] : m_stages) {
			AddStage(type, stage.filepath, stage.defines);
		}

		Init();

		ActivateProgram();
		for (auto& [name, id] : m_uniforms) {
			id = CreateUniform(name);
		}
	}

	void Shader::Init() {
		unsigned int tprogramID = glCreateProgram();

		for (unsigned int handle : m_shader_handles) {
			UseShader(handle, tprogramID);
		}
		m_shader_handles.clear();

		m_program_id = tprogramID;

		glLinkProgram(m_program_id);

		int result;
		glGetProgramiv(m_program_id, GL_LINK_STATUS, &result);

		if (result == GL_FALSE) {
			int length;
			glGetProgramiv(m_program_id, GL_INFO_LOG_LENGTH, &length);
			char* message = (char*)_malloca(length * sizeof(char));
			glGetProgramInfoLog(m_program_id, length, &length, message);


			ORNG_CORE_CRITICAL("Failed to link program for shader '{0}' : '{1}", m_name, message);
			BREAKPOINT;
			Reload();
		}

		glValidateProgram(m_program_id);
		glUseProgram(m_program_id);
	}


	int Shader::CreateUniform(const std::string& name) {
		constexpr bool SHADER_DEBUG_MODE = true;

		int location = glGetUniformLocation(m_program_id, name.c_str());
		if (location == -1 && SHADER_DEBUG_MODE) {
			ORNG_CORE_ERROR("Could not find uniform '{0}'", name);
			//BREAKPOINT;
		}

		return location;
	}


	void Shader::CompileShader(unsigned int shader_type, const std::string& source, unsigned int& shader_id) {
		shader_id = glCreateShader(shader_type);
		const char* src = source.c_str();
		glShaderSource(shader_id, 1, &src, nullptr);
		glCompileShader(shader_id);

		int result;
		glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);

		if (result == GL_FALSE) {
			int length;
			glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &length);
			char* message = (char*)alloca(length * sizeof(char));
			glGetShaderInfoLog(shader_id, length, &length, message);

			std::string shader_type_name;

			switch (shader_type) {
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

			std::string msg_copy = message;
			std::regex error_code_pattern(R"(\((\d+)\))");
			std::sregex_iterator err_code_it(msg_copy.begin(), msg_copy.end(), error_code_pattern);
			std::sregex_iterator err_code_end;

			std::vector<unsigned> err_codes;
			while (err_code_it != err_code_end) {
				err_codes.push_back(std::stoi((*err_code_it)[1]));
				err_code_it++;
			}

			ORNG_CORE_CRITICAL("Failed to compile {0} shader '{1}': {2}", shader_type_name, m_name, message);
			std::string formatted_src;
			size_t pos = 0;
			unsigned line_num = 0;
			while (pos != std::string::npos) {
				auto next = source.find("\n", pos + 1);
				if (next == std::string::npos)
					break;
				if (VectorContains(err_codes, line_num))
					formatted_src += std::to_string(line_num) + ": " + source.substr(pos + 1, next - pos - 1) + "     |---------------------ERROR NEAR LINE---------------------|" + "\n";
				else
					formatted_src += std::to_string(line_num) + ": " + source.substr(pos + 1, next - pos - 1) + "\n";

				line_num++;
				pos = source.find("\n", pos + 1);
			}

			ORNG_CORE_ERROR(formatted_src);
		}
	}


	void Shader::UseShader(unsigned int& id, unsigned int program) {
		glAttachShader(program, id);
		glDeleteShader(id);
	}

	Shader* ShaderVariants::AddVariant(unsigned id, const std::vector<std::string>& defines, const std::vector<std::string>& uniforms) {
		ASSERT(!m_shaders.contains(id));
		m_shaders[id] = Shader(std::format("{}", m_name));
		for (auto& [type, path] : m_shader_paths) {
#ifdef ORNG_RUNTIME
			// Shaders will be deserialized and loaded from a shader package
			ShaderData data{ .name = m_name, .stage = (uint32_t)type, .id = id };
			unsigned shader_handle;
			m_shaders[id].CompileShader(type, Renderer::GetShaderLibrary().PopShaderCodeFromCache(data), shader_handle);
			m_shaders[id].m_shader_handles.push_back(shader_handle);
#else
			m_shaders[id].AddStage(type, path, defines);
#endif
		}

		m_shaders[id].Init();
		m_shaders[id].AddUniforms(uniforms);

		return &m_shaders[id];
	}
}