#pragma once
#include "rendering/Textures.h"
#include "util/Log.h"
#include "core/Window.h"

namespace ORNG {

	class Framebuffer {
	public:
		Framebuffer() = default;
		Framebuffer(unsigned int id, const char* name) : m_name(name), m_framebuffer_id(id) { };
		virtual ~Framebuffer();
		void Init();
		void Bind() const;


		const Texture2D& Add2DTexture(const std::string& name, unsigned int attachment_point, const Texture2DSpec& spec);

		// Binds an existing texture to the framebuffer, texture will not be deleted upon framebuffer deletion
		void AddShared2DTexture(const std::string& name, Texture2D& tex, GLenum attachment_point);

		const Texture2DArray& Add2DTextureArray(const std::string& name, const Texture2DArraySpec& spec);

		const TextureCubemap& AddCubemapTexture(const std::string& name, const TextureCubemapSpec& spec);


		void AddRenderbuffer(unsigned int width = Window::GetWidth(), unsigned int height = Window::GetHeight());

		void BindTextureLayerToFBAttachment(unsigned int tex_ref, unsigned int attachment, unsigned int layer);

		void EnableReadBuffer(unsigned int buffer);

		void EnableDrawBuffers(unsigned int amount, unsigned int buffers[]);

		template<std::derived_from<TextureBase> T>
		T& GetTexture(const std::string& name) {
			if (!m_textures.contains(name)) {
				OAR_CORE_ERROR("No texture with name '{0}' found in framebuffer '{1}'", name, m_name);
				BREAKPOINT;
			}

			TextureBase* tex = m_textures[name].p_texture;
			bool is_valid_type = true;

			if constexpr (std::is_same<T, Texture2D>::value) {
				if (tex->m_texture_target != GL_TEXTURE_2D)
					is_valid_type = false;
			}
			else if constexpr (std::is_same<T, Texture2DArray>::value) {
				if (tex->m_texture_target != GL_TEXTURE_2D_ARRAY)
					is_valid_type = false;
			}
			else if constexpr (std::is_same<T, TextureCubemap>::value) {
				if (tex->m_texture_target != GL_TEXTURE_CUBE_MAP)
					is_valid_type = false;
			}


			if (!is_valid_type) {
				OAR_CORE_ERROR("Invalid type specified for GetTexture, texture name: '{0}'", name);
				BREAKPOINT;
			}

			return *static_cast<T*>(tex);
		};


	private:
		GLuint m_framebuffer_id;
		const char* m_name = "Unnamed framebuffer";

		struct FramebufferTexture {
			FramebufferTexture() = default;
			FramebufferTexture(TextureBase* texture, bool shared) : p_texture(texture), is_shared(shared) {};

			TextureBase* p_texture = nullptr;
			bool is_shared = false;

		};

		std::unordered_map <std::string, FramebufferTexture> m_textures;
		GLuint m_fbo = 0;
		GLuint m_rbo = 0;
	};

}