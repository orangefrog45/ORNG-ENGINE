#pragma once
#include "rendering/VAO.h"

namespace ORNG {
	class GL_StateManager {
	public:

		static void InitGL() {
			Get().I_InitGL();
		}

		inline static void BindVAO(unsigned int vao) {
			Get().IBindVAO(vao);
		}


		inline static void BindSSBO(unsigned int ssbo, unsigned int binding_index) {
			Get().IBindSSBO(ssbo, binding_index);
		}

		//Force mode will make the texture active even if it is bound to the specified unit already, use for tex parameter changes etc
		inline static void BindTexture(int target, int texture, int tex_unit, bool force_mode = false) {
			Get().IBindTexture(target, texture, tex_unit, force_mode);
		};

		// Returns the texture unit the texture is bound to if it is bound e.g GL_TEXTURE0, otherwise returns -1
		inline static int IsTextureBound(unsigned tex_obj_handle) {
			int ret = -1;
			for (auto [unit, data] : Get().m_current_texture_bindings) {
				if ((data.tex_obj) == tex_obj_handle) {
					ret = unit;
					break;
				}
			}

			return ret;
		}

		inline static unsigned GetActiveShaderProgram() {
			Get().m_current_active_shader_handle;
		}

		inline static void ActivateShaderProgram(unsigned int shader_handle) {
			Get().IActivateShaderProgram(shader_handle);
		}

		inline static void DispatchCompute(int x, int y, int z) {
			ASSERT(x < 48'000 && x > 0);
			glDispatchCompute(x, y, z);
		}

		inline static void ClearDepthBits() {
			glClear(GL_DEPTH_BUFFER_BIT);
		}

		inline static void ClearColorBits() {
			glClear(GL_COLOR_BUFFER_BIT);
		}

		[[nodiscard]] static unsigned int GenBuffer() {
			unsigned int handle;
			glCreateBuffers(1, &handle);
			return handle;
		}

		inline static void BindBuffer(unsigned int buffer_type, unsigned int buffer_handle) {
			glBindBuffer(buffer_type, buffer_handle);
		}

		inline static void ClearBitsUnsignedInt() {
			static unsigned int clear_color[] = { 0, 0, 0, 0 };
			glClearBufferuiv(GL_COLOR, 0, clear_color);
		}

		inline static void DefaultClearBits() {
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		}

		struct UniformBindingPoints {
			static const int PVMATRICES = 0;
			static const int GLOBAL_LIGHTING = 1;
			static const int GLOBALS = 2;
		};

		struct SSBO_BindingPoints {
			static const int TRANSFORMS = 0;
			static const int POINT_LIGHTS_SHADOWLESS = 1;
			static const int POINT_LIGHTS_SHADOW = 2;
			static const int SPOT_LIGHTS_SHADOWLESS = 3;
			static const int SPOT_LIGHTS_SHADOW = 4;
			static const int PARTICLE_EMITTERS = 5;
			static const int PARTICLES = 6;
			static const int PARTICLE_APPEND = 7;
			static const int SECONDARY_PARTICLES = 8;

		};

		struct TextureUnits {
			static const int COLOUR = GL_TEXTURE1;
			static const int ROUGHNESS = GL_TEXTURE2;
			static const int DIR_SHADOW_MAP = GL_TEXTURE3;
			static const int SPOT_SHADOW_MAP = GL_TEXTURE4;
			static const int POINT_SHADOW_MAP = GL_TEXTURE5;
			static const int VIEW_DEPTH = GL_TEXTURE6;
			static const int NORMAL_MAP = GL_TEXTURE7;
			static const int DIFFUSE_ARRAY = GL_TEXTURE8;
			static const int DISPLACEMENT = GL_TEXTURE9;
			static const int NORMAL_ARRAY = GL_TEXTURE10;
			static const int BLUE_NOISE = GL_TEXTURE11;
			static const int SHADER_IDS = GL_TEXTURE12;
			static const int COLOUR_CUBEMAP = GL_TEXTURE13;
			static const int COLOUR_2 = GL_TEXTURE14;
			static const int DATA_3D = GL_TEXTURE15;
			static const int DEPTH = GL_TEXTURE16;
			static const int METALLIC = GL_TEXTURE17;
			static const int AO = GL_TEXTURE18;
			static const int ROUGHNESS_METALLIC_AO = GL_TEXTURE19;
			static const int DIFFUSE_PREFILTER = GL_TEXTURE20;
			static const int SPECULAR_PREFILTER = GL_TEXTURE21;
			static const int BRDF_LUT = GL_TEXTURE22;
			static const int COLOUR_3 = GL_TEXTURE23;
			static const int BLOOM = GL_TEXTURE24;
			static const int EMISSIVE = GL_TEXTURE25;
			static const int POINTLIGHT_DEPTH = GL_TEXTURE26;
		};

		struct TextureUnitIndexes {
			static const int COLOUR = 1;
			static const int SPECULAR = 2;
			static const int DIR_SHADOW_MAP = 3;
			static const int SPOT_SHADOW_MAP = 4;
			static const int POINT_SHADOW_MAP = 5;
			static const int VIEW_DEPTH = 6;
			static const int NORMAL_MAP = 7;
			static const int DIFFUSE_ARRAY = 8;
			static const int DISPLACEMENT = 9;
			static const int NORMAL_ARRAY = 10;
			static const int BLUE_NOISE = 11;
			static const int SHADER_IDS = 12;
			static const int COLOR_CUBEMAP = 13;
			static const int COLOUR_2 = 14;
			static const int DATA_3D = 15;
			static const int ROUGHNESS_METALLIC_AO = 19;
			static const int COLOUR_3 = 23;
		};

	private:

		void I_InitGL();
		void IActivateShaderProgram(unsigned int shader_handle);

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

		int m_current_active_shader_handle = -1;
		int m_current_bound_vao = -1;
	};
}