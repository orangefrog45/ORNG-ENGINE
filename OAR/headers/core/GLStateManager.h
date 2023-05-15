#pragma once
#include "rendering/VAO.h"

namespace ORNG {
	class GL_StateManager {
	public:
		inline static void BindVAO(unsigned int vao) {
			Get().IBindVAO(vao);
		}

		inline static void BindVAO(const VAO& vao) {
			Get().IBindVAO(vao.m_vao_handle);
		}

		inline static void BindSSBO(unsigned int ssbo, unsigned int binding_index) {
			Get().IBindSSBO(ssbo, binding_index);
		}

		//Force mode will make the texture active even if it is bound to the specified unit already, use for tex parameter changes etc
		static void BindTexture(int target, int texture, int tex_unit, bool force_mode = false) {
			Get().IBindTexture(target, texture, tex_unit, force_mode);
		};

		static void ClearDepthBits() {
			glClear(GL_DEPTH_BUFFER_BIT);
		}

		static void ClearColorBits() {
			glClear(GL_COLOR_BUFFER_BIT);
		}

		[[nodiscard]] static unsigned int GenBuffer() {
			unsigned int handle;
			glGenBuffers(1, &handle);
			return handle;
		}

		static void BindBuffer(unsigned int buffer_type, unsigned int buffer_handle) {
			glBindBuffer(buffer_type, buffer_handle);
		}

		static void ClearBitsUnsignedInt() {
			static unsigned int clear_color[] = { 0, 0, 0, 0 };
			glClearBufferuiv(GL_COLOR, 0, clear_color);
		}

		static void DefaultClearBits() {
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		}

		struct UniformBindingPoints {
			static const int PVMATRICES = 0;
			static const int GLOBAL_LIGHTING = 1;
			static const int MATERIALS = 3;
		};

		struct SSBO_BindingPoints {
			static const int TRANSFORMS = 0;
			static const int POINT_LIGHTS = 1;
			static const int SPOT_LIGHTS = 2;
		};

		struct TextureUnits {
			static const int COLOR = GL_TEXTURE1;
			static const int SPECULAR = GL_TEXTURE2;
			static const int DIR_SHADOW_MAP = GL_TEXTURE3;
			static const int SPOT_SHADOW_MAP = GL_TEXTURE4;
			static const int POINT_SHADOW_MAP = GL_TEXTURE5;
			static const int WORLD_POSITIONS = GL_TEXTURE6;
			static const int NORMAL_MAP = GL_TEXTURE7;
			static const int DIFFUSE_ARRAY = GL_TEXTURE8;
			static const int DISPLACEMENT = GL_TEXTURE9;
			static const int NORMAL_ARRAY = GL_TEXTURE10;
			static const int BLUE_NOISE = GL_TEXTURE11;
			static const int SHADER_MATERIAL_IDS = GL_TEXTURE12;
			static const int COLOR_CUBEMAP = GL_TEXTURE13;
			static const int COLOR_2 = GL_TEXTURE14;
			static const int DATA_3D = GL_TEXTURE15;
		};

		struct TextureUnitIndexes {
			static const int COLOR = 1;
			static const int SPECULAR = 2;
			static const int DIR_SHADOW_MAP = 3;
			static const int SPOT_SHADOW_MAP = 4;
			static const int POINT_SHADOW_MAP = 5;
			static const int WORLD_POSITIONS = 6;
			static const int NORMAL_MAP = 7;
			static const int DIFFUSE_ARRAY = 8;
			static const int DISPLACEMENT = 9;
			static const int NORMAL_ARRAY = 10;
			static const int BLUE_NOISE = 11;
			static const int SHADER_MATERIAL_IDS = 12;
			static const int COLOR_CUBEMAP = 13;
			static const int COLOR_2 = 14;
			static const int DATA_3D = 15;
		};

	private:

		void IBindVAO(unsigned int vao) {
			if (m_current_bound_vao != vao) {
				glBindVertexArray(vao);
				m_current_bound_vao = vao;
			}
		}

		void IBindSSBO(unsigned int ssbo, unsigned int binding_index) {
			if (m_current_ssbo_bindings.contains(binding_index) && m_current_ssbo_bindings[binding_index] == ssbo)
				return;

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_index, ssbo);
			m_current_ssbo_bindings[binding_index] = ssbo;
		}

		void IBindTexture(int target, int texture, int tex_unit, bool force_mode);

		static GL_StateManager& Get() {
			static GL_StateManager s_instance;
			return s_instance;
		}

		struct TextureBindData {
			unsigned int tex_target = 0;
			unsigned int tex_obj = 0;
		};

		std::unordered_map<unsigned int, unsigned int> m_current_ssbo_bindings; // currently bound ssbo object (value) to binding index (key)
		std::unordered_map<unsigned int, TextureBindData> m_current_texture_bindings; // currently bound texture object (TextureBindData) to texture unit (key)

		unsigned int m_current_bound_vao = 0;


	};
}