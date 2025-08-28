#include "pch/pch.h"

#include "rendering/Textures.h"
#include "util/Log.h"
#include "core/GLStateManager.h"

using namespace ORNG;

TextureBaseSpec::TextureBaseSpec() : internal_format(GL_NONE), format(GL_NONE), min_filter(GL_NONE), mag_filter(GL_NONE), wrap_params(GL_REPEAT), storage_type(GL_UNSIGNED_BYTE) {};

TextureBase::TextureBase(unsigned texture_target, const std::string& name) : Asset(name), m_texture_target(texture_target), m_name(name) {
	ASSERT(name.length() <= ORNG_MAX_NAME_SIZE);
};

TextureBase::TextureBase(unsigned texture_target, const std::string& name, uint64_t t_uuid) : Asset(name, t_uuid), m_texture_target(texture_target), m_name(name) {
	ASSERT(name.length() <= ORNG_MAX_NAME_SIZE);
};

void TextureBase::Unload() {
	if (GL_StateManager::GetPtr()) {
		if (int unit = GL_StateManager::IsTextureBound(m_texture_obj); unit != -1)
			GL_StateManager::BindTexture(m_texture_target, 0, static_cast<unsigned>(unit), true);
	}

	glDeleteTextures(1, reinterpret_cast<unsigned*>(&m_texture_obj));
	m_texture_obj = 0;
};

Texture2D::Texture2D(const std::string& _filepath) : TextureBase(GL_TEXTURE_2D, _filepath) {};
Texture2D::Texture2D(const std::string& _filepath, uint64_t t_uuid) : TextureBase(GL_TEXTURE_2D, _filepath, t_uuid) {};
Texture3D::Texture3D(const std::string& name) : TextureBase(GL_TEXTURE_3D, name) {};
Texture2DArray::Texture2DArray(const std::string& name) : TextureBase(GL_TEXTURE_2D_ARRAY, name) {};
TextureCubemap::TextureCubemap(const char* name) : TextureBase(GL_TEXTURE_CUBE_MAP, name) {};
TextureCubemapArray::TextureCubemapArray(const char* name) : TextureBase(GL_TEXTURE_CUBE_MAP_ARRAY, name) {};

bool TextureBase::LoadFloatImageFile(const std::string& _filepath, unsigned int target, const TextureBaseSpec* base_spec) {
	stbi_set_flip_vertically_on_load(1);

	int width = 0;
	int	height = 0;
	int	bpp = 0;

	float* image_data = stbi_loadf(_filepath.c_str(), &width, &height, &bpp, 0);

	if (image_data == nullptr) {
		ORNG_CORE_ERROR("Can't load texture from '{0}', - '{1}'", _filepath.c_str(), stbi_failure_reason());
		return  false;
	}

	GL_StateManager::BindTexture(m_texture_target, m_texture_obj, GL_TEXTURE0, true);
	int internal_format;
	GLenum format;

	switch (bpp) {
	case 1:
		internal_format = GL_R32F;
		format = GL_RED;
		break;
	case 2:
		internal_format = GL_RG32F;
		format = GL_RG;
		break;
	case 3:
		internal_format = GL_RGB32F;
		format = GL_RGB;
		break;
	case 4:
		internal_format = GL_RGBA32F;
		format = GL_RGBA;
		break;
	default:
		ORNG_CORE_ERROR("Failed loading texture from '{0}', unsupported number of channels", _filepath);
		stbi_image_free(image_data);
		return  false;
	}

	glTexImage2D(target, 0, internal_format, width, height, 0, format, GL_FLOAT, image_data);

	if (base_spec->generate_mipmaps)
		glGenerateMipmap(m_texture_target);

	GL_StateManager::BindTexture(m_texture_target, 0, GL_TEXTURE0, true);

	stbi_image_free(image_data);

	return true;
}

bool TextureBase::LoadImageFile(const std::string& _filepath, unsigned int target, const TextureBaseSpec* base_spec) {
	stbi_set_flip_vertically_on_load(1);

	int width = 0;
	int	height = 0;
	int	bpp = 0;

	unsigned char* image_data = stbi_load(_filepath.c_str(), &width, &height, &bpp, 0);

	if (image_data == nullptr) {
		ORNG_CORE_ERROR("Can't load texture from '{0}', - '{1}'", _filepath.c_str(), stbi_failure_reason());
		return  false;
	}

	int internal_format;
	GLenum format;

	switch (bpp) {
	case 1:
		internal_format = GL_R8;
		format = GL_RED;
		break;
	case 2:
		internal_format = GL_RG8;
		format = GL_RG;
		break;
	case 3:
		if (base_spec->srgb_space)
			internal_format = GL_SRGB8;
		else
			internal_format = GL_RGB8;

		format = GL_RGB;
		break;
	case 4:
		if (base_spec->srgb_space)
			internal_format = GL_SRGB8_ALPHA8;
		else
			internal_format = GL_RGBA8;

		format = GL_RGBA;
		break;
	default:
		ORNG_CORE_ERROR("Failed loading texture from '{0}', unsupported number of channels", _filepath);
		stbi_image_free(image_data);
		return false;
	}

	GL_StateManager::BindTexture(m_texture_target, m_texture_obj, GL_TEXTURE0, true);

	glTexImage2D(target, 0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, image_data);

	if (base_spec->generate_mipmaps)
		glGenerateMipmap(m_texture_target);

	GL_StateManager::BindTexture(m_texture_target, 0, GL_TEXTURE0, true);

	stbi_image_free(image_data);

	return true;
}

void TextureBase::GenerateMips() {
	GL_StateManager::BindTexture(m_texture_target, m_texture_obj, GL_TEXTURE0, true);
	glGenerateMipmap(m_texture_target);
	GL_StateManager::BindTexture(m_texture_target, 0, GL_TEXTURE0, true);
}

bool Texture2D::LoadFromBinary(std::byte* p_data, size_t size, bool is_decompressed, int width, int height, int bpp, bool is_float) {
	stbi_set_flip_vertically_on_load(1);

	stbi_uc* image_data = is_decompressed ?
		reinterpret_cast<stbi_uc*>(p_data) :
		stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(p_data), static_cast<int>(size), &width, &height, &bpp, 0);

	if (image_data == nullptr) {
		ORNG_CORE_ERROR("Can't load binary texture data, - '{0}'", stbi_failure_reason());
		return false;
	}

	int internal_format = 0;
	unsigned format = 0;

	if (!is_float) {
		switch (bpp) {
		case 1:
			internal_format = GL_R8;
			format = GL_RED;
			break;
		case 2:
			internal_format = GL_RG8;
			format = GL_RG;
			break;
		case 3:
			if (m_spec.srgb_space)
				internal_format = GL_SRGB8;
			else
				internal_format = GL_RGB8;

			format = GL_RGB;
			break;
		case 4:
			if (m_spec.srgb_space)
				internal_format = GL_SRGB8_ALPHA8;
			else
				internal_format = GL_RGBA8;

			format = GL_RGBA;
			break;
		default:
			ORNG_CORE_ERROR("Failed loading binary texture', unsupported number of channels");
			stbi_image_free(image_data);
			return false;
		}
	}

	GL_StateManager::BindTexture(m_texture_target, m_texture_obj, GL_TEXTURE0, true);

	// Texture is expected to already be configured properly for the floating-point format
	if (is_float)
		glTexImage2D(m_texture_target, 0, m_spec.internal_format, width, height, 0, m_spec.format, GL_FLOAT, image_data);
	else
		glTexImage2D(m_texture_target, 0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, image_data);


	if (m_spec.generate_mipmaps)
		glGenerateMipmap(m_texture_target);
	GL_StateManager::BindTexture(m_texture_target, 0, GL_TEXTURE0, true);

	if (!p_data) // Memory is owned
		stbi_image_free(image_data);

	return true;
}

bool Texture2D::LoadFromFile() {
	if (m_spec.filepath.empty()) {
		ORNG_CORE_ERROR("2D Texture failed loading from file: Invalid spec");
		return false;
	}

	bool ret;
	if (m_spec.storage_type == GL_FLOAT)
		ret = LoadFloatImageFile(m_spec.filepath, GL_TEXTURE_2D, static_cast<TextureBaseSpec*>(&m_spec));
	else
		ret = LoadImageFile(m_spec.filepath, GL_TEXTURE_2D, static_cast<TextureBaseSpec*>(&m_spec));

	SetFilterAndWrapParams(false, m_spec.min_filter, m_spec.mag_filter, m_spec.wrap_params);

	return ret;
}



bool Texture2DArray::LoadFromFile() {
	if (m_spec.filepaths.empty()) {
		ORNG_CORE_ERROR("Texture2DArray failed loading: Invalid spec");
		return false;
	}

	GL_StateManager::BindTexture(m_texture_target, m_texture_obj, GL_TEXTURE0, true);
	glTexImage3D(m_texture_target, 0, m_spec.internal_format, m_spec.width, m_spec.height,
		m_spec.filepaths.size(), 0, m_spec.format, m_spec.storage_type, nullptr);

	for (size_t i = 0; i < m_spec.filepaths.size(); i++) {
		int width = 0;
		int	height = 0;
		int	bpp = 0;

		unsigned char* image_data = stbi_load(m_spec.filepaths[i].c_str(), &width, &height, &bpp, 4);

		if (image_data == nullptr) {
			ORNG_CORE_ERROR("Can't load texture from '{0}', - '{1}'", m_spec.filepaths[i].c_str(), stbi_failure_reason());
			return false;
		}

		if (width != m_spec.width || height != m_spec.height) {
			ORNG_CORE_ERROR("2D Texture loading error: width/height mismatch");
			stbi_image_free(image_data);
			return false;
		}

		glTexSubImage3D(m_texture_target, 0, 0, 0, i, width, height, 1, m_spec.format, m_spec.storage_type, image_data);

		stbi_image_free(image_data);
	}

	SetFilterAndWrapParams(false, m_spec.min_filter, m_spec.mag_filter, m_spec.wrap_params);

	return true;
}



void TextureBase::SetFilterAndWrapParams(bool is_3d, int min_filter, int mag_filter, int wrap_mode) {
	wrap_mode = wrap_mode == GL_NONE ? GL_REPEAT : wrap_mode;
	glTexParameteri(m_texture_target, GL_TEXTURE_MIN_FILTER, min_filter == GL_NONE ? GL_NEAREST : min_filter);
	glTexParameteri(m_texture_target, GL_TEXTURE_MAG_FILTER, mag_filter == GL_NONE ? GL_NEAREST : mag_filter);
	glTexParameteri(m_texture_target, GL_TEXTURE_WRAP_S, wrap_mode);
	glTexParameteri(m_texture_target, GL_TEXTURE_WRAP_R, wrap_mode);

	if (is_3d)
		glTexParameteri(m_texture_target, GL_TEXTURE_WRAP_T, wrap_mode);
}



bool TextureCubemap::LoadFromFile() {
	for (unsigned int i = 0; i < m_spec.filepaths.size(); i++) {
		if (m_spec.storage_type == GL_FLOAT)
			LoadFloatImageFile(m_spec.filepaths[i], GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, static_cast<TextureBaseSpec*>(&m_spec));
		else
			LoadImageFile(m_spec.filepaths[i], GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, static_cast<TextureBaseSpec*>(&m_spec));
	}

	SetFilterAndWrapParams(true, m_spec.min_filter, m_spec.mag_filter, m_spec.wrap_params);

	return true;
}



bool Texture2D::SetSpec(const Texture2DSpec& spec) {
	if (m_texture_obj == 0) glGenTextures(1, &m_texture_obj);

	if (spec.filepath.size() <= ORNG_MAX_FILEPATH_SIZE) {
		m_spec = spec;
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_texture_obj, GL_TEXTURE0, true);

		SetFilterAndWrapParams(false, m_spec.min_filter, m_spec.mag_filter, m_spec.wrap_params);

		if (m_spec.internal_format != GL_NONE && m_spec.format != GL_NONE)
			glTexImage2D(GL_TEXTURE_2D, 0, spec.internal_format, spec.width, spec.height, 0,
				spec.format, spec.storage_type, nullptr);

		if (m_spec.generate_mipmaps)
			glGenerateMipmap(m_texture_target);

		GL_StateManager::BindTexture(GL_TEXTURE_2D, 0, GL_TEXTURE0, true);
		filepath = m_spec.filepath;
		return true;
	}
	else {
		ORNG_CORE_ERROR("Texture2D failed setting spec: Invalid spec");
		return false;
	}

}

bool Texture2DArray::SetSpec(const Texture2DArraySpec& spec) {
	if (m_texture_obj == 0) glGenTextures(1, &m_texture_obj);

	if (std::ranges::all_of(spec.filepaths, [](const std::string& s) {return s.size() <= ORNG_MAX_FILEPATH_SIZE; })) {
		m_spec = spec;
		GL_StateManager::BindTexture(GL_TEXTURE_2D_ARRAY, m_texture_obj, GL_TEXTURE0, true);

		SetFilterAndWrapParams(false, m_spec.min_filter, m_spec.mag_filter, m_spec.wrap_params);

		if (m_spec.internal_format != GL_NONE && m_spec.format != GL_NONE)
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, m_spec.internal_format, m_spec.width, m_spec.height,
				m_spec.layer_count, 0, m_spec.format, m_spec.storage_type, nullptr);

		if (m_spec.generate_mipmaps)
			glGenerateMipmap(m_texture_target);

		return true;
	}
	else {
		ORNG_CORE_ERROR("Texture2DArray failed setting spec: Invalid spec");
		return false;
	}
}

bool Texture3D::SetSpec(const Texture3DSpec& spec) {
	if (m_texture_obj == 0) glGenTextures(1, &m_texture_obj);

	if (std::ranges::all_of(spec.filepaths, [](const std::string& s) {return s.size() <= ORNG_MAX_FILEPATH_SIZE; })) {
		m_spec = spec;
		GL_StateManager::BindTexture(GL_TEXTURE_3D, m_texture_obj, GL_TEXTURE0, true);

		SetFilterAndWrapParams(true, m_spec.min_filter, m_spec.mag_filter, m_spec.wrap_params);

		if (m_spec.internal_format != GL_NONE && m_spec.format != GL_NONE)
			glTexImage3D(GL_TEXTURE_3D, 0, m_spec.internal_format, m_spec.width, m_spec.height, m_spec.layer_count, 0, m_spec.format, m_spec.storage_type, nullptr);

		if (m_spec.generate_mipmaps)
			glGenerateMipmap(m_texture_target);

		return true;
	}
	else {
		ORNG_CORE_ERROR("3D Texture failed setting spec: Invalid spec");
		return false;
	}
}

bool TextureCubemap::SetSpec(const TextureCubemapSpec& spec) {
	if (m_texture_obj == 0) glGenTextures(1, &m_texture_obj);

	if (std::ranges::all_of(spec.filepaths, [](const std::string& s) {return s.size() <= ORNG_MAX_FILEPATH_SIZE; })) {
		m_spec = spec;
		GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, m_texture_obj, GL_TEXTURE0, true);

		ASSERT(m_spec.internal_format != GL_NONE && m_spec.format != GL_NONE);

		for (unsigned int i = 0; i < 6; i++) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, spec.internal_format, spec.width,
				spec.height, 0, spec.format, spec.storage_type, nullptr);
		}

		SetFilterAndWrapParams(true, m_spec.min_filter, m_spec.mag_filter, m_spec.wrap_params);

		if (m_spec.generate_mipmaps)
			glGenerateMipmap(m_texture_target);

		return true;
	}
	else {
		ORNG_CORE_ERROR("TextureCubemap failed setting spec: Invalid spec");
		return false;
	}
}

bool TextureCubemapArray::SetSpec(const TextureCubemapArraySpec& spec) {
	if (m_texture_obj == 0) glGenTextures(1, &m_texture_obj);

	m_spec = spec;
	GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_texture_obj, GL_TEXTURE0, true);

	ASSERT(m_spec.internal_format != GL_NONE && m_spec.format != GL_NONE);

	glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, spec.internal_format, spec.width, spec.height, spec.layer_count * 6, 0, spec.format, spec.storage_type, nullptr);

	SetFilterAndWrapParams(true, m_spec.min_filter, m_spec.mag_filter, m_spec.wrap_params);

	if (m_spec.generate_mipmaps)
		glGenerateMipmap(m_texture_target);

	GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_TEXTURE0, true);
	return true;
}

FullscreenTexture2D::FullscreenTexture2D(glm::vec2 screen_size_ratio) : Texture2D(""), m_screen_size_ratio(screen_size_ratio) {
	m_window_event_listener.OnEvent = [this](const Events::WindowEvent& _event) {
		if (_event.event_type == Events::WindowEvent::EventType::WINDOW_RESIZE) {
			OnWindowResize(_event.new_window_size);
			if (OnResize) OnResize();
		}
	};
};

// spec.width and spec.height WILL BE USED, they should match the window dimensions if needed.
bool FullscreenTexture2D::SetSpec(const Texture2DSpec& spec) {
	const bool result = Texture2D::SetSpec(spec);
	if (m_window_event_listener.m_entt_handle == entt::null)
		Events::EventManager::RegisterListener(m_window_event_listener);

	return result;
}

void FullscreenTexture2D::OnWindowResize(glm::uvec2 new_dim) {
	m_spec.width = static_cast<int>(ceil(static_cast<float>(new_dim.x) * m_screen_size_ratio.x));
	m_spec.height = static_cast<int>(ceil(static_cast<float>(new_dim.y) * m_screen_size_ratio.y));
	SetSpec(m_spec);
}
