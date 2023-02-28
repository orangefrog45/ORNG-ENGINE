#include <iostream>
#include <stb/stb_image.h>
#include <string>
#include "util/util.h"
#include "Texture.h"

Texture::Texture(unsigned int textureTarget, const std::string& filename) : m_textureTarget(textureTarget), m_filename(filename) {
}

Texture::~Texture() {
	GLCall(glDeleteTextures(1, &m_textureObj));
}

bool Texture::Load() {
	stbi_set_flip_vertically_on_load(1);
	int width = 0;
	int	height = 0;
	int	bpp = 0;
	int mode = GL_RGB;
	int internal_mode = GL_RGB;
	bool ret = true;

	unsigned char* image_data = stbi_load(m_filename.c_str(), &width, &height, &bpp, 0);

	if (image_data == NULL) {
		printf("Can't load texture from '%s' - '%s \n", m_filename.c_str(), stbi_failure_reason);
		ret = false;
	}

	GLCall(glGenTextures(1, &m_textureObj));
	GLCall(glBindTexture(m_textureTarget, m_textureObj));

	if (m_textureTarget == GL_TEXTURE_2D) {

		if (m_filename.find(".png") != std::string::npos) {
			mode = GL_RGBA8;
			internal_mode = GL_RGBA;
		}
		else {
			mode = GL_RGB8;
			internal_mode = GL_RGB;
		}


		if (bpp == 1) {
			GLCall(glTexImage2D(m_textureTarget, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, image_data))
		}
		else {
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			GLCall(glTexImage2D(m_textureTarget, 0, mode, width, height, 0, internal_mode, GL_UNSIGNED_BYTE, image_data));
		}

		/*switch (bpp) {
		case 1:
			GLCall(glTexImage2D(m_textureTarget, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, image_data));
			break;
		case 3:
			GLCall(glTexImage2D(m_textureTarget, 0, mode, width, height, 0, mode, GL_UNSIGNED_BYTE, image_data));
			break;
		default:
			std::cout << "Unsupported bit depth" << std::endl;
		}*/

	}
	else {
		printf("Unsupported texture target %x\n", m_textureTarget);
		ret = false;
	}

	GLCall(glTexParameterf(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameterf(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameterf(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameterf(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	stbi_image_free(image_data);

	GLCall(glBindTexture(m_textureTarget, 0));
	return ret;

}

unsigned int Texture::GetTextureTarget() const {
	return m_textureTarget;
}

unsigned int Texture::LoadCubeMap(std::vector<const char*> faces) {

	unsigned int textureID;
	unsigned char* image_data;

	GLCall(glGenTextures(1, &textureID));
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, textureID));

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

	for (unsigned int i = 0; i < faces.size(); i++) {

		image_data = stbi_load(faces[i], &width, &height, &bpp, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, mode, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);

		if (image_data == nullptr) {
			printf("Can't load texture from '%s' - '%s \n", faces[i], stbi_failure_reason);
			exit(3);
		}

		stbi_image_free(image_data);

	}
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));

	return textureID;
}

void Texture::Bind(unsigned int textureUnit) const {
	glActiveTexture(textureUnit);
	glBindTexture(m_textureTarget, m_textureObj);
}