#pragma once
#include "rendering/VAO.h"

namespace ORNG {
	class GL_StateManager {
	public:
		static GL_StateManager& Get() {
			DEBUG_ASSERT(mp_instance);
			return *mp_instance;
		}

		static GL_StateManager* GetPtr() {
			return mp_instance;
		}

		static void Init(GL_StateManager* p_instance = nullptr) {
			if (p_instance) {
				ASSERT(!mp_instance);
				mp_instance = p_instance;
			}
			else {
				mp_instance = new GL_StateManager();
			}
		};

		static void InitGL() {
			Get().I_InitGL();
		}

		static void InitGlew() {
			if (const unsigned glew_init_result = glewInit(); GLEW_OK != glew_init_result)
			{
				ORNG_CORE_CRITICAL("Failed to initialized glew: '{}'", reinterpret_cast<const char*>(glewGetErrorString(glew_init_result)));
			}

			Get().m_glew_initialized = true;
			const std::string gl_version{ reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)) };

			int major = -1;
			int minor = -1;
			for (const char c : gl_version) {
				if (std::isdigit(c)) {
					if (major == -1) {
						major = CharToInt(c);
					}
					else {
						minor = CharToInt(c);
						break;
					}
				}
			}

			if (major < 4 || (major == 4 && minor < 6)) {
				ORNG_CORE_CRITICAL("Minimum opengl version required is 4.6, GPU only supports up to {}.{}", major, minor);
				BREAKPOINT;
			}
		}

		inline static void BindVAO(unsigned int vao) {
			Get().IBindVAO(vao);
		}

		inline static void DeleteBuffer(unsigned buffer_handle) {
			glDeleteBuffers(1, &buffer_handle);
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
				if (data.tex_obj == tex_obj_handle) {
					ret = static_cast<int>(unit);
					break;
				}
			}

			return ret;
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

		inline static void ClearBitsUnsignedInt(unsigned r = 0, unsigned g = 0, unsigned b = 0, unsigned a = 0) {
			static unsigned int clear_color[] = { r, g, b, a };
			glClearBufferuiv(GL_COLOR, 0, clear_color);
		}

		inline static void DefaultClearBits() {
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		}

		inline static bool IsGlewIntialized() {
			return Get().m_glew_initialized;
		}

		inline static void Shutdown() {
			delete mp_instance;
			mp_instance = nullptr;
		}

		struct UniformBindingPoints {
			static constexpr int PVMATRICES = 0;
			static constexpr int GLOBAL_LIGHTING = 1;
			static constexpr int GLOBALS = 2;
		};

		struct SSBO_BindingPoints {
			static constexpr int TRANSFORMS = 0;
			static constexpr int POINT_LIGHTS = 1;
			static constexpr int SPOT_LIGHTS = 2;
			static constexpr int PARTICLE_EMITTERS = 5;
			static constexpr int PARTICLES = 6;
		};

		struct TextureUnits {
			static constexpr int COLOUR = GL_TEXTURE1;
			static constexpr int ROUGHNESS = GL_TEXTURE2;
			static constexpr int DIR_SHADOW_MAP = GL_TEXTURE3;
			static constexpr int SPOT_SHADOW_MAP = GL_TEXTURE4;
			static constexpr int POINT_SHADOW_MAP = GL_TEXTURE5;
			static constexpr int VIEW_DEPTH = GL_TEXTURE6;
			static constexpr int NORMAL_MAP = GL_TEXTURE7;
			static constexpr int DIFFUSE_ARRAY = GL_TEXTURE8;
			static constexpr int DISPLACEMENT = GL_TEXTURE9;
			static constexpr int NORMAL_ARRAY = GL_TEXTURE10;
			static constexpr int BLUE_NOISE = GL_TEXTURE11;
			static constexpr int SHADER_IDS = GL_TEXTURE12;
			static constexpr int COLOUR_CUBEMAP = GL_TEXTURE13;
			static constexpr int COLOUR_2 = GL_TEXTURE14;
			static constexpr int DATA_3D = GL_TEXTURE15;
			static constexpr int DEPTH = GL_TEXTURE16;
			static constexpr int METALLIC = GL_TEXTURE17;
			static constexpr int AO = GL_TEXTURE18;
			static constexpr int ROUGHNESS_METALLIC_AO = GL_TEXTURE19;
			static constexpr int DIFFUSE_PREFILTER = GL_TEXTURE20;
			static constexpr int SPECULAR_PREFILTER = GL_TEXTURE21;
			static constexpr int BRDF_LUT = GL_TEXTURE22;
			static constexpr int COLOUR_3 = GL_TEXTURE23;
			static constexpr int BLOOM = GL_TEXTURE24;
			static constexpr int EMISSIVE = GL_TEXTURE25;
			static constexpr int POINTLIGHT_DEPTH = GL_TEXTURE26;
			static constexpr int SCENE_VOXELIZATION = GL_TEXTURE27;
		};

		struct TextureUnitIndexes {
			static constexpr int COLOUR = 1;
			static constexpr int SPECULAR = 2;
			static constexpr int DIR_SHADOW_MAP = 3;
			static constexpr int SPOT_SHADOW_MAP = 4;
			static constexpr int POINT_SHADOW_MAP = 5;
			static constexpr int VIEW_DEPTH = 6;
			static constexpr int NORMAL_MAP = 7;
			static constexpr int DIFFUSE_ARRAY = 8;
			static constexpr int DISPLACEMENT = 9;
			static constexpr int NORMAL_ARRAY = 10;
			static constexpr int BLUE_NOISE = 11;
			static constexpr int SHADER_IDS = 12;
			static constexpr int COLOR_CUBEMAP = 13;
			static constexpr int COLOUR_2 = 14;
			static constexpr int DATA_3D = 15;
			static constexpr int ROUGHNESS_METALLIC_AO = 19;
			static constexpr int COLOUR_3 = 23;
		};

	private:
		void I_InitGL();

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

		inline static GL_StateManager* mp_instance = nullptr;

		bool m_glew_initialized = false;

		struct TextureBindData {
			unsigned int tex_target = 0;
			unsigned int tex_obj = 0;
		};

		std::unordered_map<unsigned int, unsigned int> m_current_ssbo_bindings; // currently bound ssbo object (value) to binding index (key)
		std::unordered_map<unsigned int, TextureBindData> m_current_texture_bindings; // currently bound texture object (TextureBindData) to texture unit (key)

		unsigned m_current_active_shader_handle = 0;
		unsigned m_current_bound_vao = 0;
	};
}