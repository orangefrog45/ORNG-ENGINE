#include <iostream>
#include <stb/stb_image.h>
#include "util.h"
#include "Texture.h"

Texture::Texture(unsigned int textureTarget, const std::string& filename) : m_textureTarget(textureTarget), m_filename(filename) {
}

bool Texture::Load() {
	stbi_set_flip_vertically_on_load(1);
	int width = 0;
	int	height = 0;
	int	bpp = 0;
	int mode = GL_RGB;

	unsigned char* image_data = stbi_load(m_filename.c_str(), &width, &height, &bpp, 0);
	printf("width: %d", width);

		if (image_data == nullptr) {
			printf("Can't load texture from '%s' - '%s \n", m_filename.c_str(), stbi_failure_reason);
			exit(0);
		}

	GLCall(glGenTextures(1, &m_textureObj));
	GLCall(glBindTexture(m_textureTarget, m_textureObj));

	if (m_textureTarget == GL_TEXTURE_2D) {
		glTexImage2D(m_textureTarget, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
	}
	else {
		printf("Unsupported texture target %x\n", m_textureTarget);
	}

	GLCall(glTexParameterf(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameterf(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameterf(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP));
	GLCall(glTexParameterf(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP));

	stbi_image_free(image_data);

	GLCall(glBindTexture(m_textureTarget, 0));
	return true;

}

void Texture::Bind(unsigned int textureUnit) const {
	glActiveTexture(textureUnit);
	glBindTexture(m_textureTarget, m_textureObj);
}