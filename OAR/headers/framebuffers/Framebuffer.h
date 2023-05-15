#pragma once
#include "rendering/Textures.h"
#include "util/Log.h"

namespace ORNG {

	class Framebuffer {
	public:
		Framebuffer() = default;
		Framebuffer(unsigned int id, const char* name) : m_name(name), m_framebuffer_id(id) { };


		const Texture2D& Add2DTexture(const std::string& name, unsigned int attachment_point, const Texture2DSpec& spec);

		const Texture2DArray& Add2DTextureArray(const std::string& name, const Texture2DArraySpec& spec);


		void AddRenderbuffer();

		void BindTextureLayerToFBAttachment(unsigned int tex_ref, unsigned int attachment, unsigned int layer);

		void EnableReadBuffer(unsigned int buffer);

		void EnableDrawBuffers(unsigned int amount, unsigned int buffers[]);

		template<std::derived_from<TextureBase> T>
		T& GetTexture(const std::string& name) {
			if (!m_textures.contains(name)) {
				OAR_CORE_ERROR("No texture with name '{0}' found in framebuffer '{1}'", name, m_name);
				BREAKPOINT;
			}

			TextureBase* tex = m_textures[name];
			bool is_valid_type = true;

			if constexpr (std::is_same<T, Texture2D>::value) {
				if (tex->m_texture_target != GL_TEXTURE_2D)
					is_valid_type = false;

			}
			else if constexpr (std::is_same<T, Texture2DArray>::value) {
				if (tex->m_texture_target != GL_TEXTURE_2D_ARRAY)
					is_valid_type = false;


			}


			if (!is_valid_type) {
				OAR_CORE_ERROR("Invalid type specified for GetTexture, texture name: '{0}'", name);
				BREAKPOINT;
			}

			return *static_cast<T*>(tex);
		};

		virtual ~Framebuffer();
		void Init();
		void Bind() const;
	protected:
		unsigned int m_framebuffer_id;
		const char* m_name = "Unnamed framebuffer";
		std::unordered_map <std::string, TextureBase*> m_textures;
		unsigned int m_fbo = 0;
		unsigned int m_rbo = 0;
	};

}