#pragma once
#include "util/util.h"
#include "util/UUID.h"
#include "util/Log.h"
#include "bitsery/traits/string.h"
#include "bitsery/traits/core/traits.h"

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

	// Struct containing raw texture data loaded from files, data will automatically be freed when this goes out of scope
	struct TextureFileData {
		// Default constructor returns invalid data - this will be handled by the engine.
		TextureFileData() : data_type(INVALID) {};
		TextureFileData(unsigned char* p_data, uint32_t  t_width, uint32_t t_height, uint8_t t_channels) : data_8_bit(p_data), data_type(BIT8), width(t_width), height(t_height), channels(t_channels) {};
		TextureFileData(float* p_data, uint32_t  t_width, uint32_t t_height, uint8_t t_channels) : data_32_bit(p_data), data_type(BIT32), width(t_width), height(t_height), channels(t_channels) {};
		TextureFileData(TextureFileData&& other) = default;

		TextureFileData& operator=(TextureFileData&& other) noexcept {
			if (this != &other) {
				data_8_bit = other.data_8_bit;
				data_32_bit = other.data_32_bit;
				data_type = other.data_type;
			}
			return *this;
		}

		~TextureFileData() {
			/*if (data_type == BIT8)
				//stbi_image_free(data_8_bit);
			else if (data_type == BIT32)

				//stbi_image_free(data_32_bit);*/
		}

		template <typename S>
		void serialize(S& s) {
			if (data_type == BIT8) {
				for (size_t i = 0; i < (size_t)width * (size_t)height * (size_t)channels; i++) {
					s.value1b(*((uint8_t*)data_8_bit + i));
				}
			}
			else if (data_type == BIT32) {
				for (size_t i = 0; i < (size_t)width * (size_t)height * (size_t)channels; i++) {
					s.value4b(*((float*)data_32_bit + i));
				}
			}


			s.value4b(width);
			s.value4b(height);
			s.value1b((uint8_t)data_type);
			s.value1b(channels);
		}

		unsigned char* data_8_bit = nullptr;
		float* data_32_bit = nullptr;
		enum DataType {
			INVALID,
			BIT8,
			BIT32
		};

		DataType data_type = INVALID;
		uint32_t width = 0;
		uint32_t height = 0;
		uint8_t channels = 0;
	};
	class TextureBase {

	public:
		friend class Framebuffer;
		TextureBase() = delete;
		virtual ~TextureBase() { Unload(); };

		void Unload();

		unsigned int GetTextureHandle() const { return m_texture_obj; }

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
		std::unique_ptr<TextureFileData> LoadFloatImageFile(const std::string& filepath, unsigned int target, const TextureBaseSpec* base_spec, unsigned int layer = 0);
		std::unique_ptr<TextureFileData> LoadImageFile(const std::string& filepath, unsigned int  target, const TextureBaseSpec* base_spec, unsigned int layer = 0);
		TextureBase(unsigned int texture_target, const std::string& name);
		TextureBase(unsigned int texture_target, const std::string& name, uint64_t t_uuid);
		uint32_t m_texture_target = 0;
		uint32_t m_texture_obj = 0;
		std::string m_name = "Unnamed texture";

	};

	struct Texture2DSpec : public TextureBaseSpec {
		template <typename S>
		void serialize(S& s) {
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
		Texture2D(const std::string& name);
		Texture2D(const std::string& name, uint64_t t_uuid);
		// Allocates a new texture object and copies texture data from other
		Texture2D(const Texture2D& other);
		Texture2D& operator=(const Texture2D& other);

		bool SetSpec(const Texture2DSpec& spec);
		std::unique_ptr<TextureFileData> LoadFromFile();
		const Texture2DSpec& GetSpec() const { return m_spec; }

		template <typename S>
		void serialize(S& s) {
			s.object(m_spec);
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