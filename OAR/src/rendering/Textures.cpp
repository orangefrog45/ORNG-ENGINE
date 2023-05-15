#include "pch/pch.h"

#include "rendering/Textures.h"
#include "util/Log.h"
#include "core/GLStateManager.h"

namespace ORNG {

	Texture2D::Texture2D() : TextureBase(GL_TEXTURE_2D) {
	}


	bool Texture2D::LoadFromFile() {

		if (!ValidateSpec(m_spec) || m_spec.filepath.empty()) {
			OAR_CORE_ERROR("2D Texture failed loading: Invalid spec");
			return false;
		}

		if (m_texture_obj != 0) {
			OAR_CORE_WARN("Texture2D containing '{0}'...  is already loaded, overwriting...", m_spec.filepath);
		}

		stbi_set_flip_vertically_on_load(1);
		int width = 0;
		int	height = 0;
		int	bpp = 0;

		unsigned char* image_data = stbi_load(m_spec.filepath.c_str(), &width, &height, &bpp, 0);

		if (image_data == nullptr) {
			OAR_CORE_ERROR("Can't load texture from '{0}', - '{1}'", m_spec.filepath.c_str(), stbi_failure_reason());
		}

		GL_StateManager::BindTexture(m_texture_target, m_texture_obj, GL_TEXTURE0, true);
		glTexImage2D(m_texture_target, 0, m_spec.internal_format, width, height, 0, m_spec.format, m_spec.storage_type, image_data);

		GLenum wrap_mode = m_spec.wrap_params == GL_NONE ? GL_REPEAT : m_spec.wrap_params;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_spec.min_filter == GL_NONE ? GL_NEAREST : m_spec.min_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_spec.mag_filter == GL_NONE ? GL_NEAREST : m_spec.mag_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_mode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_mode);

		if (m_spec.generate_mipmaps)
			glGenerateMipmap(m_texture_target);

		stbi_image_free(image_data);
		return true;
	}



	Texture2DArray::Texture2DArray() : TextureBase(GL_TEXTURE_2D_ARRAY)
	{
	};


	bool Texture2DArray::LoadFromFile() {

		if (m_texture_obj != 0) {
			OAR_CORE_WARN("Texture2DArray containing '{0}'...  is already loaded, overwriting...", m_spec.filepaths[0]);
		}

		if (!ValidateSpec(m_spec) || m_spec.filepaths.empty()) {
			OAR_CORE_ERROR("Texture2DArray failed loading: Invalid spec");
			return false;
		}


		stbi_set_flip_vertically_on_load(1);

		GL_StateManager::BindTexture(m_texture_target, m_texture_obj, GL_TEXTURE0, true);

		glTexImage3D(m_texture_target, 0, m_spec.internal_format, m_spec.width, m_spec.height, m_spec.filepaths.size(), 0, m_spec.format, m_spec.storage_type, nullptr);


		for (int i = 0; i < m_spec.filepaths.size(); i++) {

			int width = 0;
			int	height = 0;
			int	bpp = 0;

			unsigned char* image_data = stbi_load(m_spec.filepaths[i].c_str(), &width, &height, &bpp, 4);

			if (image_data == nullptr) {
				OAR_CORE_ERROR("Can't load texture from '{0}', - '{1}'", m_spec.filepaths[i].c_str(), stbi_failure_reason());
				return false;
			}

			if (width != m_spec.width || height != m_spec.height) {
				OAR_CORE_ERROR("2D Texture loading error: width/height mismatch");
				stbi_image_free(image_data);
				return false;
			}

			glTexSubImage3D(m_texture_target, 0, 0, 0, i, width, height, 1, m_spec.format, m_spec.storage_type, image_data);

			stbi_image_free(image_data);
		}


		GLenum wrap_mode = m_spec.wrap_params == GL_NONE ? GL_REPEAT : m_spec.wrap_params;
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, m_spec.min_filter == GL_NONE ? GL_NEAREST : m_spec.min_filter);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, m_spec.mag_filter == GL_NONE ? GL_NEAREST : m_spec.mag_filter);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, wrap_mode);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, wrap_mode);

		if (m_spec.generate_mipmaps)
			glGenerateMipmap(m_texture_target);

		return true;
	}





	TextureCubemap::TextureCubemap() : TextureBase(GL_TEXTURE_CUBE_MAP)
	{
	};




	bool TextureCubemap::LoadFromFile() {

		if (!ValidateSpec(m_spec) || m_spec.filepaths.size() != 6) {
			OAR_CORE_ERROR("TextureCubemap failed to load, invalid spec");
			return false;
		}

		if (m_texture_obj != 0) {
			OAR_CORE_WARN("TextureCubemap containing '{0}'... is already loaded, overwriting", m_spec.filepaths[0]);
		}

		GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, m_texture_obj, GL_TEXTURE0, true);

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

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, m_spec.internal_format, width, height, 0, m_spec.format, m_spec.storage_type, image_data);

			stbi_image_free(image_data);

		}

		GLenum wrap_mode = m_spec.wrap_params == GL_NONE ? GL_REPEAT : m_spec.wrap_params;
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, m_spec.min_filter == GL_NONE ? GL_NEAREST : m_spec.min_filter);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, m_spec.mag_filter == GL_NONE ? GL_NEAREST : m_spec.mag_filter);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrap_mode);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, wrap_mode);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrap_mode);

		if (m_spec.generate_mipmaps)
			glGenerateMipmap(m_texture_target);


		return true;
	}

	bool Texture2D::ValidateSpec(const Texture2DSpec& spec) {
		if (spec.width == 1 || spec.height == 1) {
			OAR_CORE_WARN("Texture2D has default height/width of 1px, this will be changed to fit if loading texture from a file");
		}

		if (spec.internal_format == GL_NONE || spec.format == GL_NONE) {
			OAR_CORE_ERROR("Texture2D failed setting spec: Invalid spec");
			return false;
		}
		else {
			return true;
		}
	}

	bool Texture2DArray::ValidateSpec(const Texture2DArraySpec& spec) {
		if (spec.width == 1 || spec.height == 1) {
			OAR_CORE_WARN("Texture2DArray has default height/width of 1px/1px, this will be changed to fit if loading texture from a file");
		}

		if (spec.internal_format == GL_NONE || spec.format == GL_NONE) {
			OAR_CORE_ERROR("Texture2DArray failed setting spec: Invalid spec");
			return false;
		}
		else {
			return true;
		}
	}


	bool Texture3D::ValidateSpec(const Texture3DSpec& spec) {
		if (spec.width == 1 || spec.height == 1) {
			OAR_CORE_WARN("Texture3D has default height/width of 1px/1px, this will be changed to fit if loading texture from a file");
		}

		if (spec.internal_format == GL_NONE || spec.format == GL_NONE) {
			OAR_CORE_ERROR("Texture3D failed setting spec: Invalid spec");
			return false;
		}
		else {
			return true;
		}
	}
	bool TextureCubemap::ValidateSpec(const TextureCubemapSpec& spec) {
		if (spec.width == 1 || spec.height == 1) {
			OAR_CORE_WARN("TextureCubemap has default height/width of 1px/1px, this will be changed to fit if loading texture from a file");
		}

		if (spec.internal_format == GL_NONE || spec.format == GL_NONE) {
			OAR_CORE_ERROR("TextureCubemap failed setting spec: Invalid spec");
			return false;
		}
		else {
			return true;
		}
	}

	bool Texture2D::SetSpec(const Texture2DSpec& spec, bool should_allocate_space) {
		if (ValidateSpec(spec)) {
			m_spec = spec;
			GL_StateManager::BindTexture(GL_TEXTURE_2D, m_texture_obj, GL_TEXTURE0, true);

			GLenum wrap_mode = m_spec.wrap_params == GL_NONE ? GL_REPEAT : m_spec.wrap_params;
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_spec.min_filter == GL_NONE ? GL_NEAREST : m_spec.min_filter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_spec.mag_filter == GL_NONE ? GL_NEAREST : m_spec.mag_filter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_mode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_mode);

			if (should_allocate_space)
				glTexImage2D(GL_TEXTURE_2D, 0, m_spec.internal_format, spec.width, spec.height, 0, m_spec.format, m_spec.storage_type, nullptr);

			return true;
		}
		else {
			OAR_CORE_ERROR("Texture2D failed setting spec: Invalid spec");
			return false;
		}
	}

	bool Texture2DArray::SetSpec(const Texture2DArraySpec& spec) {
		if (ValidateSpec(spec)) {
			m_spec = spec;
			GL_StateManager::BindTexture(GL_TEXTURE_2D_ARRAY, m_texture_obj, GL_TEXTURE0, true);

			GLenum wrap_mode = m_spec.wrap_params == GL_NONE ? GL_REPEAT : m_spec.wrap_params;
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, m_spec.min_filter == GL_NONE ? GL_NEAREST : m_spec.min_filter);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, m_spec.mag_filter == GL_NONE ? GL_NEAREST : m_spec.mag_filter);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, wrap_mode);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, wrap_mode);

			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, m_spec.internal_format, m_spec.width, m_spec.height, m_spec.layer_count, 0, m_spec.format, m_spec.storage_type, nullptr);
			return true;
		}
		else {
			OAR_CORE_ERROR("Texture2DArray failed setting spec: Invalid spec");
			return false;
		}
	}

	bool Texture3D::SetSpec(const Texture3DSpec& spec) {
		if (ValidateSpec(spec)) {
			m_spec = spec;
			GL_StateManager::BindTexture(GL_TEXTURE_3D, m_texture_obj, GL_TEXTURE0, true);

			GLenum wrap_mode = m_spec.wrap_params == GL_NONE ? GL_REPEAT : m_spec.wrap_params;
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, m_spec.min_filter == GL_NONE ? GL_NEAREST : m_spec.min_filter);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, m_spec.mag_filter == GL_NONE ? GL_NEAREST : m_spec.mag_filter);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, wrap_mode);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, wrap_mode);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, wrap_mode);

			glTexImage3D(GL_TEXTURE_3D, 0, m_spec.internal_format, m_spec.width, m_spec.height, m_spec.layer_count, 0, m_spec.format, m_spec.storage_type, nullptr);
			return true;
		}
		else {
			OAR_CORE_ERROR("3D Texture failed setting spec: Invalid spec");
			return false;
		}
	}


	bool TextureCubemap::SetSpec(const TextureCubemapSpec& spec) {
		if (ValidateSpec(spec)) {
			m_spec = spec;
			GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, m_texture_obj, GL_TEXTURE0, true);


			for (unsigned int i = 0; i < 6; i++) {
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, spec.internal_format, spec.width, spec.height, 0, spec.format, spec.storage_type, nullptr);
			}

			GLenum wrap_mode = m_spec.wrap_params == GL_NONE ? GL_REPEAT : m_spec.wrap_params;
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, m_spec.min_filter == GL_NONE ? GL_NEAREST : m_spec.min_filter);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, m_spec.mag_filter == GL_NONE ? GL_NEAREST : m_spec.mag_filter);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrap_mode);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, wrap_mode);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrap_mode);
			return true;
		}
		else {
			OAR_CORE_ERROR("TextureCubemap failed setting spec: Invalid spec");
			return false;
		}
	}
}