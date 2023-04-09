#include <stb/stb_image.h>
#include "TextureCubemap.h"
#include "util/util.h"
#include "Log.h"

TextureCubemap::TextureCubemap() : TextureBase(GL_TEXTURE_CUBE_MAP) {};

TextureCubemap::TextureCubemap(const std::vector<const char*>& faces) : TextureBase(GL_TEXTURE_CUBE_MAP), m_faces(faces) {};

void TextureCubemap::Load() {

	ASSERT(m_faces.size() == 6);

	unsigned char* image_data;

	GLCall(glGenTextures(1, &m_texture_obj));
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture_obj));


	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));



	stbi_set_flip_vertically_on_load(0);
	int width = 0;
	int	height = 0;
	int	bpp = 0;

	for (unsigned int i = 0; i < m_faces.size(); i++) {

		image_data = stbi_load(m_faces[i], &width, &height, &bpp, 0);
		if (image_data == nullptr) {
			OAR_CORE_ERROR("Can't load texture from '{0}', - '{1}'", m_faces[i], stbi_failure_reason());
		}
		int mode = GL_RGB;
		int internal_mode = GL_RGB8;

		if (std::string(m_faces[i]).find(".png") != std::string::npos) {
			internal_mode = GL_RGBA8;
			mode = GL_RGBA;
		}
		else {
			internal_mode = GL_RGB8;
			mode = GL_RGB;
		}
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internal_mode, width, height, 0, mode, GL_UNSIGNED_BYTE, image_data);

		stbi_image_free(image_data);

	}
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
}