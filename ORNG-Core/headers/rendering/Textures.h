#pragma once
#include "assets/Asset.h"
#include "events/EventManager.h"
#include "util/util.h"
#include "util/Log.h"

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

		int internal_format;
		uint32_t format;
		int min_filter;
		int mag_filter;
		int width = 1;
		int height = 1;

		int wrap_params = GL_CLAMP_TO_EDGE;
		uint32_t storage_type;

		uint8_t generate_mipmaps = false;
		uint8_t srgb_space = false;
	};


	class TextureBase : public Asset {
		friend class GL_StateManager;
	public:
		friend class Framebuffer;
		TextureBase() = delete;
		~TextureBase() override { Unload(); }

		void Unload();
		void GenerateMips();
		[[nodiscard]] unsigned GetTextureHandle() const noexcept { return m_texture_obj; }
		[[nodiscard]] unsigned GetTarget() const noexcept { return m_texture_target; }
		[[nodiscard]] const std::string& GetName() const noexcept { return m_name; }

		void SetName(const std::string& name) noexcept { m_name = name; }

		void SetFilterAndWrapParams(bool is_3d, int min_filter, int mag_filter, int wrap_mode);

		template <typename S>
		void serialize(S& s) {
			s.object(uuid);
			s.text1b(m_name, ORNG_MAX_NAME_SIZE);
			s.value4b(m_texture_target);
		}

	protected:
		bool LoadFloatImageFile(const std::string& filepath, unsigned int target, const TextureBaseSpec* base_spec);
		bool LoadImageFile(const std::string& filepath, unsigned int  target, const TextureBaseSpec* base_spec);
		TextureBase(unsigned texture_target, const std::string& name);
		TextureBase(unsigned texture_target, const std::string& name, uint64_t t_uuid);
		unsigned m_texture_target = 0;
		unsigned m_texture_obj = 0;

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
		explicit Texture3D(const std::string& name);

		bool SetSpec(const Texture3DSpec& spec);
		[[nodiscard]] const Texture3DSpec& GetSpec() const noexcept { return m_spec; }
	private:
		Texture3DSpec m_spec;

	};


	class Texture2D : public TextureBase {
	public:
		friend class EditorLayer;
		friend class AssetManager;
		friend class AssetSerializer;

		explicit Texture2D(const std::string& filepath);
		Texture2D(const std::string& filepath, uint64_t t_uuid);

		virtual bool SetSpec(const Texture2DSpec& spec);
		bool LoadFromFile();
		bool LoadFromBinary(std::byte* p_data, size_t size, bool is_decompressed, int width = -1, int height = -1, int channels = -1, bool is_float = false);
		[[nodiscard]] const Texture2DSpec& GetSpec() const noexcept { return m_spec; }

	protected:
		Texture2DSpec m_spec;
	};


	// Texture that automatically resizes itself upon window resize to match (window dimensions * screen_size_ratio).
	// Previous contents are erased.
	class FullscreenTexture2D final : public Texture2D {
	public:
		// screen_size_ratio - this texture will be resized to (new_window_dimensions * screen_size_ratio), should be 1 if the texture needs to match window size.
		explicit FullscreenTexture2D(glm::vec2 screen_size_ratio = {1, 1});

		// spec.width and spec.height WILL BE USED, they should match the window dimensions if needed.
		bool SetSpec(const Texture2DSpec& spec) final;

		// A callback that is invoked immediately after this texture resizes
		std::function<void()> OnResize = nullptr;
	private:
		void OnWindowResize(glm::uvec2 new_dim);

		glm::vec2 m_screen_size_ratio;
		Events::EventListener<Events::WindowEvent> m_window_event_listener;
	};


	class Texture2DArray : public TextureBase {
	public:
		explicit Texture2DArray(const std::string& name);

		bool LoadFromFile();
		bool SetSpec(const Texture2DArraySpec& spec);
		[[nodiscard]] const Texture2DArraySpec& GetSpec() const noexcept { return m_spec; }
	private:
		Texture2DArraySpec m_spec;
	};


	class TextureCubemap : public TextureBase {
		friend class Renderer;
	public:
		explicit TextureCubemap(const char* name);
		bool SetSpec(const TextureCubemapSpec& spec);
		bool LoadFromFile();
		[[nodiscard]] const TextureCubemapSpec& GetSpec() const noexcept { return m_spec; }
	private:
		TextureCubemapSpec m_spec;
	};

	class TextureCubemapArray : public TextureBase {
		friend class Renderer;
	public:
		explicit TextureCubemapArray(const char* name);
		bool SetSpec(const TextureCubemapArraySpec& spec);
		[[nodiscard]] const TextureCubemapArraySpec& GetSpec() const noexcept { return m_spec; }
	private:
		TextureCubemapArraySpec m_spec;
	};
}
