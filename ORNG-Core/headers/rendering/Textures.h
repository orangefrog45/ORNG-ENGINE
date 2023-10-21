#pragma once
#include "util/util.h"
#include "util/UUID.h"
#include "util/Log.h"
#include "bitsery/traits/string.h"
#include "bitsery/traits/core/traits.h"
#include "assets/Asset.h"

namespace ORNG {

	struct Texture2DSpec;
	struct Texture3DSpec;
	struct Texture2DArraySpec;
	struct TextureCubemapSpec;
	struct TextureCubemapArraySpec;

	struct TextureBaseSpec {
		TextureBaseSpec();

		template <typename S>
		void serialize(S& s) {
			s.value4b(internal_format);
			s.value4b(format);
			s.value4b(min_filter);
			s.value4b(mag_filter);
			s.value4b(width);
			s.value4b(height);
			s.value4b(wrap_params);
			s.value4b(storage_type);
			s.value1b(generate_mipmaps);
			s.value1b(srgb_space);
		}

		uint32_t internal_format;
		uint32_t format;
		uint32_t min_filter;
		uint32_t mag_filter;
		uint32_t width = 1;
		uint32_t height = 1;

		uint32_t wrap_params;
		uint32_t storage_type;

		uint8_t generate_mipmaps = false;
		uint8_t srgb_space = false;
	};


	class TextureBase : public Asset {
		friend class GL_StateManager;
	public:
		friend class Framebuffer;
		TextureBase() = delete;
		virtual ~TextureBase() { Unload(); };

		void Unload();

		unsigned int GetTextureHandle() const { return m_texture_obj; }
		unsigned int GetTarget() const { return m_texture_target; }

		void GenerateMips();

		bool ValidateBaseSpec(const TextureBaseSpec* spec, bool is_framebuffer_texture = false);

		const std::string& GetName() {
			return m_name;
		}

		template <typename S>
		void serialize(S& s) {
			s.object(uuid);
			s.text1b(m_name, ORNG_MAX_NAME_SIZE);
			s.value4b(m_texture_target);
		}

		UUID uuid;
	protected:
		bool LoadFloatImageFile(const std::string& filepath, unsigned int target, const TextureBaseSpec* base_spec, unsigned int layer = 0);
		bool LoadImageFile(const std::string& filepath, unsigned int  target, const TextureBaseSpec* base_spec, unsigned int layer = 0);
		TextureBase(unsigned int texture_target, const std::string& name);
		TextureBase(unsigned int texture_target, const std::string& name, uint64_t t_uuid);
		uint32_t m_texture_target = 0;
		uint32_t m_texture_obj = 0;

		// When texture is bound, texture unit e.g GL_TEXTURE0 stored here
		uint32_t m_binding_point = 0;

		std::string m_name = "Unnamed texture";

	};

	struct Texture2DSpec : public TextureBaseSpec {
		template <typename S>
		void serialize(S& s) {
			TextureBaseSpec::serialize(s);
			s.text1b(filepath, ORNG_MAX_FILEPATH_SIZE);
		}
		std::string filepath;
	};

	struct Texture2DArraySpec : public TextureBaseSpec {
		std::vector<std::string> filepaths;
		unsigned int layer_count = 1;
	};

	struct TextureCubemapSpec : public TextureBaseSpec {
		std::array<std::string, 6> filepaths;
	};

	struct TextureCubemapArraySpec : public TextureBaseSpec {
		unsigned int layer_count = 1;
	};

	struct Texture3DSpec : public Texture2DArraySpec {};

	class Texture3D : public TextureBase {
	public:
		friend class Scene;
		Texture3D(const std::string& name);

		bool SetSpec(const Texture3DSpec& spec);
		const Texture3DSpec& GetSpec() const { return m_spec; }
	private:
		Texture3DSpec m_spec;

	};


	class Texture2D : public TextureBase {
	public:
		friend class EditorLayer;
		friend class Scene;
		Texture2D(const std::string& filepath);
		Texture2D(const std::string& filepath, uint64_t t_uuid);
		// Allocates a new texture object and copies texture data from other
		Texture2D(const Texture2D& other);
		Texture2D& operator=(const Texture2D& other);

		bool SetSpec(const Texture2DSpec& spec);
		bool LoadFromFile();
		const Texture2DSpec& GetSpec() const { return m_spec; }

		template <typename S>
		void serialize(S& s) {
			s.object(m_spec);
			s.object(uuid);
			s.text1b(filepath, ORNG_MAX_FILEPATH_SIZE);
		}

	private:
		Texture2DSpec m_spec;
	};




	class Texture2DArray : public TextureBase {
	public:
		Texture2DArray(const std::string& name);
		bool LoadFromFile();

		bool SetSpec(const Texture2DArraySpec& spec);
		const Texture2DArraySpec& GetSpec() const { return m_spec; }

	private:
		Texture2DArraySpec m_spec;
	};


	class TextureCubemap : public TextureBase {
		friend class Renderer;
	public:
		TextureCubemap(const char* name);
		bool SetSpec(const TextureCubemapSpec& spec);
		bool LoadFromFile();
		const TextureCubemapSpec& GetSpec() const { return m_spec; }
	private:
		TextureCubemapSpec m_spec;
	};

	class TextureCubemapArray : public TextureBase {
		friend class Renderer;
	public:
		TextureCubemapArray(const char* name);
		bool SetSpec(const TextureCubemapArraySpec& spec);
		const TextureCubemapArraySpec& GetSpec() const { return m_spec; }
	private:
		TextureCubemapArraySpec m_spec;
	};

}