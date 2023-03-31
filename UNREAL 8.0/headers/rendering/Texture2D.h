#pragma once
#include <glew.h>
#include <glfw/glfw3.h>
#include <string>
#include <vector>
#include "util/util.h"
#include "TextureBase.h"


class Texture2D : public TextureBase {
public:
	explicit Texture2D(const std::string& filename) : TextureBase(GL_TEXTURE_2D) { m_filename = filename; };

	bool Load() override;

};