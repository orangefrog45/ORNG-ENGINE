#pragma once
#include <string>
#include <vector>
#include <glew.h>

class Texture {
public:
	Texture() = default;
	Texture(unsigned int textureTarget, const std::string& filename);
	~Texture();

	bool Load();

	void Bind(unsigned int textureUnit) const;

	static unsigned int LoadCubeMap(std::vector<const char*> faces);

	unsigned int GetTextureTarget() const;

private:
	std::string m_filename;
	unsigned int m_textureTarget;
	unsigned int m_textureObj;

};