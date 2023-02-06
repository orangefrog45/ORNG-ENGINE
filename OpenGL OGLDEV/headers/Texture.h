#pragma once
#include <string>
#include <glew.h>

class Texture {
public:
	Texture(unsigned int textureTarget, const std::string& filename);

	bool Load();

	void Bind(unsigned int textureUnit);

private:
	std::string m_filename;
	unsigned int m_textureTarget;
	unsigned int m_textureObj;

};