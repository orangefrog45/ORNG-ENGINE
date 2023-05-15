#pragma once
#include "util/util.h"

namespace ORNG {


	class TextureBase {
	public:
		friend class Framebuffer;
		TextureBase() = delete;
		virtual ~TextureBase() { Unload(); };

		void Unload() { glDeleteTextures(1, &m_texture_obj); };

		unsigned int GetTextureHandle() const { return m_texture_obj; }

	protected:
		TextureBase(unsigned int texture_target) : m_texture_target(texture_target) { glGenTextures(1, &m_texture_obj); };
		unsigned int m_texture_target = 0;
		unsigned int m_texture_obj = 0;

	};

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
		Texture3D() : TextureBase(GL_TEXTURE_3D) {};

		bool SetSpec(const Texture3DSpec& spec);
		static bool ValidateSpec(const Texture3DSpec& spec);
		const Texture3DSpec& GetSpec() const { return m_spec; }
	private:
		Texture3DSpec m_spec;

	};


	class Texture2D : public TextureBase {
	public:
		friend class EditorLayer;
		friend class Scene;
		Texture2D();

		bool SetSpec(const Texture2DSpec& spec, bool should_allocate_space);
		static bool ValidateSpec(const Texture2DSpec& spec);
		bool LoadFromFile();
		const Texture2DSpec& GetSpec() const { return m_spec; }

	private:
		Texture2DSpec m_spec;
	};




	class Texture2DArray : public TextureBase {
	public:
		Texture2DArray();
		bool LoadFromFile();
		static bool ValidateSpec(const Texture2DArraySpec& spec);

		bool SetSpec(const Texture2DArraySpec& spec);

	private:
		Texture2DArraySpec m_spec;
	};


	class TextureCubemap : public TextureBase {
		friend class Renderer;
	public:
		TextureCubemap();
		static bool ValidateSpec(const TextureCubemapSpec& spec);
		bool SetSpec(const TextureCubemapSpec& spec);
		bool LoadFromFile();
	private:
		TextureCubemapSpec m_spec;
	};

}