#include <stb/stb_image.h>
#include <string>
#include "util/util.h"
#include "Texture2D.h"
#include "Log.h"


bool Texture2D::Load() {
	stbi_set_flip_vertically_on_load(1);
	int width = 0;
	int	height = 0;
	int	bpp = 0;
	int mode = GL_RGB;
	int internal_mode = GL_RGB;
	bool ret = true;

	unsigned char* image_data = stbi_load(m_filename.c_str(), &width, &height, &bpp, 0);

	if (image_data == NULL) {
		ORO_CORE_ERROR("Can't load texture from '{0}', - '{1}'", m_filename.c_str(), stbi_failure_reason());
		ret = false;
	}

	GLCall(glGenTextures(1, &m_texture_obj));
	GLCall(glBindTexture(m_texture_target, m_texture_obj));


	if (m_filename.find(".png") != std::string::npos) {
		internal_mode = GL_RGBA8;
		mode = GL_RGBA;

	}
	else {
		internal_mode = GL_RGB8;
		mode = GL_RGB;
	}


	if (bpp == 1) {
		GLCall(glTexImage2D(m_texture_target, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, image_data));
	}
	else {
		GLCall(glTexImage2D(m_texture_target, 0, internal_mode, width, height, 0, mode, GL_UNSIGNED_BYTE, image_data));
	}
	GLCall(glGenerateMipmap(m_texture_target));

	/*switch (bpp) {
	case 1:
		GLCall(glTexImage2D(m_texture_target, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, image_data));
		break;
	case 3:
		GLCall(glTexImage2D(m_texture_target, 0, mode, width, height, 0, mode, GL_UNSIGNED_BYTE, image_data));
		break;
	default:
		std::cout << "Unsupported bit depth" << std::endl;
	}*/


	GLCall(glTexParameteri(m_texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	GLCall(glTexParameteri(m_texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(m_texture_target, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GLCall(glTexParameteri(m_texture_target, GL_TEXTURE_WRAP_T, GL_REPEAT));

	stbi_image_free(image_data);

	GLCall(glBindTexture(m_texture_target, 0));
	return ret;

}
