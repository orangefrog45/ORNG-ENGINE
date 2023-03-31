#include <stb/stb_image.h>
#include "TextureCubemap.h"
#include "Log.h"

bool TextureCubemap::Load() {

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
	int mode = GL_RGB;

	for (unsigned int i = 0; i < m_faces.size(); i++) {

		image_data = stbi_load(m_faces[i], &width, &height, &bpp, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, mode, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);

		if (image_data == nullptr) {
			ORO_CORE_ERROR("Can't load texture from '{0}', - '{1}'", m_faces[i], stbi_failure_reason());
			return false;
		}

		stbi_image_free(image_data);

	}
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
	return true;
}