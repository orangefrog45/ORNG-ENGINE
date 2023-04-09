#pragma once
#include <vector>
#include "TextureBase.h"

class Texture2DArray : public TextureBase {
public:
	Texture2DArray(unsigned int tex_width, unsigned int tex_height);
	void Load() override;
	void AddTexture(const std::string& filename) { m_filepaths.emplace_back(filename); }
	void RemoveTexture(const std::string& filename) { m_filepaths.erase(std::find(m_filepaths.begin(), m_filepaths.end(), filename)); }
private:
	unsigned int m_tex_width;
	unsigned int m_tex_height;
	std::vector<std::string> m_filepaths;
};