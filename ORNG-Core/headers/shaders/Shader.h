#pragma once
#include "util/Log.h"
#include "util/util.h"
#include "util/UUID.h"
#include "events/Events.h"

namespace ORNG {
	class Material;

	class Shader {
	public:
		friend class ShaderLibrary;
		friend class Renderer;
		friend class SceneRenderer;
		friend class ShaderVariants;

		Shader() = default;
		explicit Shader(const std::string& name) :  m_name(name) {}
		Shader& operator=(const Shader&) = delete;
		Shader(const Shader&) = delete;
		~Shader();

		enum class ShaderType {
			NONE = -1, VERTEX = 0, FRAGMENT = 1
		};

		/* Links and validates shader program */
		void Init();

		// Compiles a shader program with preprocessor definitions in "defines"
		void AddStage(GLenum shader_type, const std::string& filepath, std::vector<std::string> defines = {});

		void Reload();

		const std::string& GetName() const {
			return m_name;
		}

		// Compiles a shader program from "shader_code" string with preprocessor definitions in "defines"
		void AddStageFromString(GLenum shader_type, const std::string& shader_code, std::vector<std::string> defines = {});

		void ActivateProgram();

		inline void AddUniform(const std::string& name) {
			ActivateProgram();
			auto handle = CreateUniform(name);

			if (handle != -1)
				m_uniforms[name] = CreateUniform(name);
		}

		template<typename... Args>
		requires((std::is_convertible_v<Args, std::string>), ...)
		void AddUniforms(Args&&... args) {
			(AddUniform(args), ...);
		}

		inline void AddUniforms(const std::vector<std::string>& names) {
			ActivateProgram();
			for (auto& uname : names) {
				AddUniform(uname);
			}
		}

		template<typename T>
		void SetUniform(const std::string& name, T value) {
			if (!m_uniforms.contains(name)) {
				//ORNG_CORE_ERROR("Uniform '{0}' not found in shader '{1}'", name, m_name);
				return;
			}

			if constexpr (std::is_same<T, float>::value) {
				glUniform1f(m_uniforms[name], value);
			}
			else if constexpr (std::is_same<T, int>::value || std::is_same<T, bool>::value) {
				glUniform1i(m_uniforms[name], value);
			}
			else if constexpr (std::is_same<T, glm::vec3>::value) {
				glUniform3f(m_uniforms[name], value.x, value.y, value.z);
			}
			else if constexpr (std::is_same<T, glm::vec2>::value) {
				glUniform2f(m_uniforms[name], value.x, value.y);
			}
			else if constexpr (std::is_same<T, glm::mat4>::value) {
				glUniformMatrix4fv(m_uniforms[name], 1, GL_FALSE, &value[0][0]);
			}
			else if constexpr (std::is_same<T, glm::mat3>::value) {
				glUniformMatrix3fv(m_uniforms[name], 1, GL_FALSE, &value[0][0]);
			}
			else if constexpr (std::is_same<T, unsigned int>::value || std::is_same<T, uint32_t>::value) {
				glUniform1ui(m_uniforms[name], value);
			}
			else if constexpr (std::is_same<T, glm::vec4>::value) {
				glUniform4f(m_uniforms[name], value.x, value.y, value.z, value.w);
			}
			else if constexpr (std::is_same<T, glm::uvec2>::value) {
				glUniform2ui(m_uniforms[name], value.x, value.y);
			}
			else if constexpr (std::is_same<T, glm::uvec3>::value) {
				glUniform3ui(m_uniforms[name], value.x, value.y, value.z);
			}
			else if constexpr (std::is_same<T, glm::ivec3>::value) {
				glUniform3i(m_uniforms[name], value.x, value.y, value.z);
			}
			else {
				ORNG_CORE_ERROR("Unsupported uniform type used in shader, uniform name: '{}', shader name '{}'", name, m_name);
			}
		}

	private:
		struct StageData {
			StageData() = default;
			StageData(const std::string& fp, const std::vector<std::string>& dfn) : filepath(fp), defines(dfn) { }
			std::string filepath;
			std::vector<std::string> defines;
		};

		int CreateUniform(const std::string& name);

		void CompileShader(unsigned int type, const std::string& source, unsigned int& shaderID);

		void UseShader(unsigned int& id, unsigned int program);

		struct ParsedShaderData {
			ParsedShaderData(std::string t_code, unsigned t_line_count) : shader_code(std::move(t_code)), line_count(t_line_count) {}
			std::string shader_code;
			unsigned line_count;
		};

		static ParsedShaderData ParseShader(const std::string& filepath, std::vector<std::string>& defines, std::vector<const std::string*>& include_tree, GLenum shader_type, unsigned line_count = 0);

		static void ParseShaderInclude(const std::string& filepath, std::vector<std::string>& defines, std::stringstream& stream, std::vector<const std::string*>& include_tree, unsigned& line_count, GLenum shader_type);

		static void ParseShaderIncludeString(const std::string& filepath, std::vector<std::string>& defines, std::string& shader_str, size_t directive_pos, std::vector<const std::string*>& include_tree, unsigned& line_count, GLenum shader_type);

		unsigned int m_program_id = 0;
		std::unordered_map<GLenum, StageData> m_stages;
		std::unordered_map<std::string, int> m_uniforms;
		std::vector<unsigned int> m_shader_handles;
		std::string m_name = "Unnamed shader";

		Events::EventListener<Events::ShaderReloadEvent> m_reload_listener;
	};


	// This class can contain multiple of the same shader with different defines
	class ShaderVariants {
		friend class ShaderLibrary;
	public:
		ShaderVariants() = default;
		ShaderVariants(const std::string& name) : m_name(name) { }

		void Activate(unsigned id) {
			ASSERT(m_shaders.contains(id));
			m_shaders[id].ActivateProgram();
			m_active_shader_id = id;
		}

		template<typename T>
		void SetUniform(const std::string& name, T value) {
			m_shaders[m_active_shader_id].SetUniform(name, value);
		}

		void SetPath(GLenum shader_stage, const std::string& path) {
			ASSERT(!m_shader_paths.contains(shader_stage));
			m_shader_paths[shader_stage] = path;
		}

		const std::string& GetName() const { return m_name; }

		// Adds a shader variant at id 'id' with the defines specified
		Shader* AddVariant(unsigned id, const std::vector<std::string>& defines, const std::vector<std::string>& uniforms);

	private:
		// Not guaranteed to be accurate, individual "Shader" objects being activated will override, just used for shortcut
		inline static unsigned m_active_shader_id = 0;

		std::string m_name;
		std::unordered_map<GLenum, std::string> m_shader_paths;
		std::unordered_map<unsigned, Shader> m_shaders;
	};
}
