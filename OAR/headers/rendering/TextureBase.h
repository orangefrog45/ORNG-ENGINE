#pragma once
#include "util/util.h"
#include "glew.h"
#include "glfw3.h"
#include <string>
class TextureBase {
public:
	TextureBase() = delete;
	virtual ~TextureBase() { Unload(); };

	virtual void Load() = 0;

	void Unload() { GLCall(glDeleteTextures(1, &m_texture_obj)); };

	unsigned int GetTextureRef() const { return m_texture_obj; }

protected:
	explicit TextureBase(unsigned int texture_target) : m_texture_target(texture_target) {};
	std::string m_filename;
	unsigned int m_texture_target = 0;
	unsigned int m_texture_obj = 0;

};