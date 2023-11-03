#pragma once
#include "util/Log.h"
#include "util/util.h"
#include "core/GLStateManager.h"

namespace ORNG {
	struct Material;

	class Shader {
	public:
		friend class ShaderLibrary;
		friend class Renderer;
		friend class SceneRenderer;
		Shader() = default;
		Shader(const char* name, unsigned int id) : m_id(id), m_name(name) {};
		~Shader();

		enum class ShaderType {
			NONE = -1, VERTEX = 0, FRAGMENT = 1
		};

		/* Links and validates shader program */
		void Init();

		// Compiles a shader program with preprocessor definitions in "defines"
		void AddStage(GLenum shader_type, const std::string& filepath, std::vector<std::string> defines = {});

		// Compiles a shader program from "shader_code" string with preprocessor definitions in "defines"
		void AddStageFromString(GLenum shader_type, const std::string& shader_code, std::vector<std::string> defines = {});

		inline void ActivateProgram() {
			GL_StateManager::ActivateShaderProgram(m_program_id);
		};

		inline void AddUniform(const std::string& name) {
			ActivateProgram();
			m_uniforms[name] = CreateUniform(name);
		};

		inline void AddUniforms(const std::vector<std::string>& names) {
			ActivateProgram();
			for (auto& uname : names) {
				m_uniforms[uname] = CreateUniform(uname);
			}
		};

		template<typename T>
		void SetUniform(const std::string& name, T value) {
			if (!m_uniforms.contains(name)) {
				ORNG_CORE_ERROR("Uniform '{0}' not found in shader '{1}'", name, m_name);
			}


			if constexpr (std::is_same<T, float>::value) {
				glUniform1f(m_uniforms[name], value);
			}
			else if constexpr (std::is_same<T, int>::value) {
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
			else {
				ORNG_CORE_ERROR("Unsupported uniform type used for shader: {0}", m_name);
			}
		};

		unsigned int GetID() const {
			return m_id;
		}
	protected:


		unsigned int CreateUniform(const std::string& name);

		void CompileShader(unsigned int type, const std::string& source, unsigned int& shaderID);

		void UseShader(unsigned int& id, unsigned int program);

		std::string ParseShader(const std::string& filepath, std::vector<std::string>& defines, std::vector<const std::string*>& include_tree);

		void ParseShaderInclude(const std::string& filepath, std::vector<std::string>& defines, std::stringstream& stream, std::vector<const std::string*>& include_tree);

		void ParseShaderIncludeString(const std::string& filepath, std::vector<std::string>& defines, std::string& str, size_t directive_pos, std::vector<const std::string*>& include_tree);

		unsigned int m_id;

		unsigned int m_program_id = 0;

		std::unordered_map<std::string, unsigned int> m_uniforms;
		std::vector<unsigned int> m_shader_handles;
		const char* m_name = "Unnamed shader";
	};
}