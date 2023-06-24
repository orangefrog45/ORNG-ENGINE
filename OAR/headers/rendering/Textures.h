#pragma once
#include "util/util.h"
#include "util/Log.h"

namespace ORNG {

	class Texture2DSpec;
	class Texture3DSpec;
	class Texture2DArraySpec;
	class TextureCubemapSpec;

	struct TextureBaseSpec {

		GLenum internal_format = GL_NONE;
		GLenum format = GL_NONE;
		GLenum min_filter = GL_NONE;
		GLenum mag_filter = GL_NONE;
		unsigned int width = 1;
		unsigned int height = 1;

		GLenum wrap_params = GL_REPEAT;
		GLenum storage_type = GL_UNSIGNED_BYTE;
		bool generate_mipmaps = false;
		bool srgb_space = false;
	};
	class TextureBase {

	public:
		friend class Framebuffer;
		TextureBase() = delete;
		virtual ~TextureBase() { Unload(); };

		void Unload() { glDeleteTextures(1, &m_texture_obj); };

		unsigned int GetTextureHandle() const { return m_texture_obj; }

		bool ValidateBaseSpec(const TextureBaseSpec* spec, bool is_framebuffer_texture = false) {

			if (is_framebuffer_texture) {
				if (spec->width == 1 || spec->height == 1)
				{
					OAR_CORE_WARN("Framebuffer texture '{0}' has default height/width of 1px, this will be changed to fit if loading texture from a file", m_name);
				}

				if (spec->internal_format == GL_NONE || spec->format == GL_NONE)
				{
					OAR_CORE_WARN("Framebuffer texture '{0}' has no internal/regular format, texture memory will not be allocated", m_name);

					if (is_framebuffer_texture)
						return false;
				}
			}

			return true;
		}

		bool LoadFloatImageFile(const std::string& filepath, GLenum target, const TextureBaseSpec* base_spec, unsigned int layer = 0);
		bool LoadImageFile(const std::string& filepath, GLenum target, const TextureBaseSpec* base_spec, unsigned int layer = 0);
	protected:
		TextureBase(unsigned int texture_target, const std::string& name) : m_texture_target(texture_target), m_name(name) { glGenTextures(1, &m_texture_obj); };
		unsigned int m_texture_target = 0;
		unsigned int m_texture_obj = 0;
		std::string m_name = "Unnamed texture";

	};

	struct Texture2DSpec : public TextureBaseSpec {
		std::string filepath;
	};

	struct Texture2DArraySpec : public TextureBaseSpec {
		std::vector<std::string> filepaths;
		unsigned int layer_count = 1;
	};

	struct TextureCubemapSpec : public TextureBaseSpec {
		std::array<std::string, 6> filepaths;
	};

	struct Texture3DSpec : public Texture2DArraySpec {};

	class Texture3D : public TextureBase {
	public:
		friend class EditorLayer;
		friend class Scene;
		Texture3D(const std::string& name) : TextureBase(GL_TEXTURE_3D, name) {};

		bool SetSpec(const Texture3DSpec& spec);
		const Texture3DSpec& GetSpec() const { return m_spec; }
	private:
		Texture3DSpec m_spec;

	};


	class Texture2D : public TextureBase {
	public:
		friend class EditorLayer;
		friend class Scene;
		Texture2D(const std::string& name) : TextureBase(GL_TEXTURE_2D, name) {};

		bool SetSpec(const Texture2DSpec& spec);
		bool LoadFromFile();
		const Texture2DSpec& GetSpec() const { return m_spec; }

	private:
		Texture2DSpec m_spec;
	};




	class Texture2DArray : public TextureBase {
	public:
		Texture2DArray(const std::string& name) : TextureBase(GL_TEXTURE_2D_ARRAY, name) {};
		bool LoadFromFile();

		bool SetSpec(const Texture2DArraySpec& spec);
		const Texture2DArraySpec& GetSpec() const { return m_spec; }

	private:
		Texture2DArraySpec m_spec;
	};


	class TextureCubemap : public TextureBase {
		friend class Renderer;
	public:
		TextureCubemap(const char* name) : TextureBase(GL_TEXTURE_CUBE_MAP, name) {};
		bool SetSpec(const TextureCubemapSpec& spec);
		bool LoadFromFile();
	private:
		TextureCubemapSpec m_spec;
	};

}