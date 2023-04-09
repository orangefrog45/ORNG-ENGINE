#include <stb/stb_image.h>
#include <string>
#include "Texture2DArray.h"
#include "util/util.h"
#include "Log.h"

Texture2DArray::Texture2DArray(unsigned int tex_width, unsigned int tex_height) : TextureBase(GL_TEXTURE_2D_ARRAY), m_tex_width(tex_width), m_tex_height(tex_height)
{

};

void Texture2DArray::Load() {
	ASSERT(m_filepaths.size() > 0);

	if (m_texture_obj != 0) {
		OAR_CORE_ERROR("Texture array containing '{0}'...   failed to load, it is already loaded", m_filepaths[0]);
		BREAKPOINT;
	}

	stbi_set_flip_vertically_on_load(1);

	GLCall(glGenTextures(1, &m_texture_obj));
	GLCall(glBindTexture(m_texture_target, m_texture_obj));


	//GLCall(glTexStorage3D(m_texture_target, m_filepaths.size(), GL_RGBA8, m_tex_width, m_tex_height, 1));
	GLCall(glTexImage3D(m_texture_target, 0, GL_RGBA8, m_tex_width, m_tex_height, m_filepaths.size(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));
	GLCall(glBindTexture(m_texture_target, m_texture_obj));
	GLCall(glTexParameteri(m_texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	GLCall(glTexParameteri(m_texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(m_texture_target, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT));
	GLCall(glTexParameteri(m_texture_target, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT));

	for (int i = 0; i < m_filepaths.size(); i++) {

		int width = 0;
		int	height = 0;
		int	bpp = 0;
		int mode = GL_RGB;
		int internal_mode = GL_RGB;

		unsigned char* image_data = stbi_load(m_filepaths[i].c_str(), &width, &height, &bpp, 4);

		if (image_data == nullptr) {
			OAR_CORE_ERROR("Can't load texture from '{0}', - '{1}'", m_filepaths[i].c_str(), stbi_failure_reason());
			return;
		}

		if (m_filepaths[i].find(".png") != std::string::npos && bpp == 4) {
			internal_mode = GL_RGBA8;
			mode = GL_RGBA;
		}
		else {
			internal_mode = GL_RGB8;
			mode = GL_RGB;
		}

		OAR_CORE_INFO(width);

		if (width != m_tex_width || height != m_tex_height) {
			OAR_CORE_ERROR("Texture 2D Array cannot be loaded, allocated texture size is [Width: {0}, Height:{1}], current texture size is [Width: {2}, Height: {3}]", m_tex_width, m_tex_height, width, height);
		}

		GLCall(glTexSubImage3D(m_texture_target, 0, 0, 0, i, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, image_data));

		stbi_image_free(image_data);
	}


	GLCall(glGenerateMipmap(m_texture_target));

	GLCall(glBindTexture(m_texture_target, 0));
}