#pragma once
#include "util/Log.h"
#include "util/util.h"
#include "core/GLStateManager.h"

namespace ORNG {

	class Material;

	class Shader {
	public:
		friend class ShaderLibrary;
		friend class Renderer;
		friend class SceneRenderer;
		Shader() = default;
		Shader(const char* name, unsigned int id) : m_name(name), m_shader_id(id) {};
		~Shader();

		enum class ShaderType {
			NONE = -1, VERTEX = 0, FRAGMENT = 1
		};

		/* Links and validates shader program */
		void Init();

		/* Return shader ID usable for setting shaders for meshes */
		inline unsigned int GetShaderID() { return m_shader_id; }

		void AddStage(GLenum shader_type, const std::string& filepath);

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
				OAR_CORE_ERROR("Uniform '{0}' not found in shader '{1}'", name, m_name);
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
			else if constexpr (std::is_same<T, glm::mat4>::value) {
				glUniformMatrix4fv(m_uniforms[name], 1, GL_FALSE, &value[0][0]);
			}
			else if constexpr (std::is_same<T, glm::mat3>::value) {
				glUniformMatrix3fv(m_uniforms[name], 1, GL_FALSE, &value[0][0]);
			}
			else if constexpr (std::is_same<T, unsigned int>::value) {
				glUniform1ui(m_uniforms[name], value);
			}
			else if constexpr (std::is_same<T, glm::vec4>::value) {
				glUniform4f(m_uniforms[name], value.x, value.y, value.z, value.w);
			}
			else {
				OAR_CORE_ERROR("Unsupported uniform type used for shader: {0}", m_name);
			}
		};
	protected:


		unsigned int CreateUniform(const std::string& name);

		void CompileShader(unsigned int type, const std::string& source, unsigned int& shaderID);

		void UseShader(unsigned int& id, unsigned int program);

		std::string ParseShader(const std::string& filepath);

		int m_shader_id = -1;

		unsigned int m_program_id;

		std::unordered_map<std::string, unsigned int> m_uniforms;
		std::vector<unsigned int> m_shader_handles;
		const char* m_name = "Unnamed shader";

	};
}