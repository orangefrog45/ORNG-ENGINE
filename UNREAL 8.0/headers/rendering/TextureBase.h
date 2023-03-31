#pragma once
#include "util/util.h"
#include <string>
class TextureBase {
public:
	virtual ~TextureBase() { Unload(); };

	virtual bool Load() = 0;

	void Unload() { GLCall(glDeleteTextures(1, &m_texture_obj)); };

	unsigned int GetTextureRef() const { return m_texture_obj; }

protected:
	explicit TextureBase(unsigned int texture_target) : m_texture_target(texture_target) {};
	std::string m_filename;
	unsigned int m_texture_target = -1;
	unsigned int m_texture_obj = -1;

};