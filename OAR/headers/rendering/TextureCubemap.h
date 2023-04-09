#pragma once
#include <vector>
#include "TextureBase.h"

class TextureCubemap : public TextureBase {
	friend class Renderer;
public:
	explicit TextureCubemap(const std::vector<const char*>& faces);
	TextureCubemap();
	void SetFaces(const std::vector<const char*>& faces) { m_faces = faces; }
	void Load();
private:
	std::vector<const char*> m_faces;
};
