#include "pch/pch.h"

#include "rendering/Textures.h"
#include "util/Log.h"

Texture2D::Texture2D(const std::string& filename) : TextureBase(GL_TEXTURE_2D) {
	m_filename = filename;
}


bool Texture2D::Load() {

	if (m_texture_obj != 0) {
		OAR_CORE_WARN("2D Texture containing '{0}'...  is already loaded", m_spec.filename);
		return true;
	}

	if (!ValidateSpec()) {
		OAR_CORE_ERROR("2D Texture failed loading: Invalid spec");
		return false;
	}

	stbi_set_flip_vertically_on_load(1);
	int width = 0;
	int	height = 0;
	int	bpp = 0;

	unsigned char* image_data = stbi_load(m_filename.c_str(), &width, &height, &bpp, 0);

	if (image_data == nullptr) {
		glDeleteTextures(1, &m_texture_obj);
		OAR_CORE_ERROR("Can't load texture from '{0}', - '{1}'", m_filename.c_str(), stbi_failure_reason());
	}

	GLCall(glGenTextures(1, &m_texture_obj));
	GLCall(glBindTexture(m_texture_target, m_texture_obj));
	GLCall(glTexImage2D(m_texture_target, 0, m_spec.internal_format, width, height, 0, m_spec.format, GL_UNSIGNED_BYTE, image_data));

	if (m_spec.generate_mipmaps)
		GLCall(glGenerateMipmap(m_texture_target));

	GLCall(glTexParameteri(m_texture_target, GL_TEXTURE_MIN_FILTER, m_spec.min_filter == GL_NONE ? GL_LINEAR : m_spec.min_filter));
	GLCall(glTexParameteri(m_texture_target, GL_TEXTURE_MAG_FILTER, m_spec.mag_filter == GL_NONE ? GL_LINEAR : m_spec.mag_filter));
	GLCall(glTexParameteri(m_texture_target, GL_TEXTURE_WRAP_S, m_spec.wrap_params == GL_NONE ? GL_REPEAT : m_spec.wrap_params));
	GLCall(glTexParameteri(m_texture_target, GL_TEXTURE_WRAP_T, m_spec.wrap_params == GL_NONE ? GL_REPEAT : m_spec.wrap_params));

	stbi_image_free(image_data);

	GLCall(glBindTexture(m_texture_target, 0));
	return true;
}



Texture2DArray::Texture2DArray() : TextureBase(GL_TEXTURE_2D_ARRAY)
{

};



bool Texture2DArray::Load() {
	if (m_spec.filepaths.empty()) {
		OAR_CORE_ERROR("2DTextureArray load failed, no files given to load");
		return false;
	}

	if (m_texture_obj != 0) {
		OAR_CORE_WARN("2DTextureArray containing '{0}'...  is already loaded", m_spec.filepaths[0]);
		return true;
	}

	if (m_spec.internal_format == GL_NONE || m_spec.format == GL_NONE || m_spec.tex_width == -1 || m_spec.tex_height == -1) {
		OAR_CORE_ERROR("2DTextureArray failed loading: Invalid spec");
		return false;
	}

	stbi_set_flip_vertically_on_load(1);

	GLCall(glGenTextures(1, &m_texture_obj));
	GLCall(glBindTexture(m_texture_target, m_texture_obj));

	GLCall(glTexImage3D(m_texture_target, 0, m_spec.internal_format, m_spec.tex_width, m_spec.tex_height, m_spec.filepaths.size(), 0, m_spec.format, GL_UNSIGNED_BYTE, nullptr));
	GLCall(glBindTexture(m_texture_target, m_texture_obj));
	GLCall(glTexParameteri(m_texture_target, GL_TEXTURE_MIN_FILTER, m_spec.min_filter == GL_NONE ? GL_LINEAR : m_spec.min_filter));
	GLCall(glTexParameteri(m_texture_target, GL_TEXTURE_MAG_FILTER, m_spec.mag_filter == GL_NONE ? GL_LINEAR : m_spec.mag_filter));
	GLCall(glTexParameteri(m_texture_target, GL_TEXTURE_WRAP_S, m_spec.wrap_params == GL_NONE ? GL_REPEAT : m_spec.wrap_params));
	GLCall(glTexParameteri(m_texture_target, GL_TEXTURE_WRAP_T, m_spec.wrap_params == GL_NONE ? GL_REPEAT : m_spec.wrap_params));

	for (int i = 0; i < m_spec.filepaths.size(); i++) {

		int width = 0;
		int	height = 0;
		int	bpp = 0;

		unsigned char* image_data = stbi_load(m_spec.filepaths[i].c_str(), &width, &height, &bpp, 4);

		if (image_data == nullptr) {
			glDeleteTextures(1, &m_texture_obj);
			OAR_CORE_ERROR("Can't load texture from '{0}', - '{1}'", m_spec.filepaths[i].c_str(), stbi_failure_reason());
			return false;
		}

		if (width != m_spec.tex_width || height != m_spec.tex_height) {
			OAR_CORE_ERROR("2D Texture loading error: width/height mismatch");
			glDeleteTextures(1, &m_texture_obj);
			stbi_image_free(image_data);
			return false;
		}

		GLCall(glTexSubImage3D(m_texture_target, 0, 0, 0, i, width, height, 1, m_spec.format, GL_UNSIGNED_BYTE, image_data));

		stbi_image_free(image_data);
	}

	if (m_spec.generate_mipmaps)
		GLCall(glGenerateMipmap(m_texture_target));

	GLCall(glBindTexture(m_texture_target, 0));
	return true;
}




TextureCubemap::TextureCubemap() : TextureBase(GL_TEXTURE_CUBE_MAP) {};




bool TextureCubemap::Load() {

	if (!ValidateSpec()) {
		OAR_CORE_ERROR("TextureCubemap failed to load, invalid spec");
		return false;
	}

	if (m_texture_obj != 0) {
		OAR_CORE_WARN("TextureCubemap containing '{0}'... is already loaded", m_spec.filepaths[0]);
		return true;
	}


	GLCall(glGenTextures(1, &m_texture_obj));
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture_obj));

	GLenum wrap_param = m_spec.wrap_params == GL_NONE ? GL_REPEAT : m_spec.wrap_params;
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, m_spec.min_filter == GL_NONE ? GL_LINEAR : m_spec.min_filter));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, m_spec.mag_filter == GL_NONE ? GL_LINEAR : m_spec.mag_filter));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrap_param));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, wrap_param));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrap_param));

	unsigned char* image_data;

	stbi_set_flip_vertically_on_load(0);
	int width = 0;
	int	height = 0;
	int	bpp = 0;

	for (unsigned int i = 0; i < m_spec.filepaths.size(); i++) {

		image_data = stbi_load(m_spec.filepaths[i].c_str(), &width, &height, &bpp, 0);

		if (image_data == nullptr) {
			OAR_CORE_ERROR("Can't load texture from '{0}', - '{1}'", m_spec.filepaths[i], stbi_failure_reason());
			return false;
		}

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, m_spec.internal_format, width, height, 0, m_spec.format, GL_UNSIGNED_BYTE, image_data);

		stbi_image_free(image_data);

	}

	if (m_spec.generate_mipmaps)
		GLCall(glGenerateMipmap(m_texture_target));


	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
	return true;
}

bool Texture2D::ValidateSpec() const {
	if (m_spec.internal_format == GL_NONE || m_spec.format == GL_NONE || m_spec.filename.empty()) {
		OAR_CORE_ERROR("Texture2D failed setting spec: Invalid spec");
		return false;
	}
	else {
		return true;
	}
}

bool Texture2DArray::ValidateSpec() const {
	if (m_spec.internal_format == GL_NONE || m_spec.format == GL_NONE || m_spec.tex_width == -1 || m_spec.tex_height == -1 || m_spec.filepaths.empty()) {
		OAR_CORE_ERROR("Texture2DArray failed setting spec: Invalid spec");
		return false;
	}
	else {
		return true;
	}
}

bool TextureCubemap::ValidateSpec() const {
	if (m_spec.internal_format == GL_NONE || m_spec.format == GL_NONE || m_spec.filepaths.size() != 6) {
		OAR_CORE_ERROR("TextureCubemap failed setting spec: Invalid spec");
		return false;
	}
	else {
		return true;
	}
}

bool Texture2D::SetSpec(const Texture2DSpec& spec) {
	m_spec = spec;
	if (!ValidateSpec()) {
		OAR_CORE_ERROR("Texture2D failed setting spec: Invalid spec");
		return false;
	}
	else {
		return true;
	}
}

bool Texture2DArray::SetSpec(const Texture2DArraySpec& spec) {
	m_spec = spec;
	if (!ValidateSpec()) {
		OAR_CORE_ERROR("Texture2DArray failed setting spec: Invalid spec");
		return false;
	}
	else {
		return true;
	}
}


bool TextureCubemap::SetSpec(const TextureCubemapSpec& spec) {
	m_spec = spec;
	if (!ValidateSpec()) {
		OAR_CORE_ERROR("TextureCubemap failed setting spec: Invalid spec");
		return false;
	}
	else {
		return true;
	}
}