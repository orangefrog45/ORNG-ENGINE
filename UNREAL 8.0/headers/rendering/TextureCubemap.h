#pragma once
#include "Texture2D.h"

class TextureCubemap : public TextureBase {
	friend class Renderer;
public:
	explicit TextureCubemap(const std::vector<const char*>& faces) : TextureBase(GL_TEXTURE_CUBE_MAP), m_faces(faces) {};
	TextureCubemap() : TextureBase(GL_TEXTURE_CUBE_MAP) {};
	void SetFaces(const std::vector<const char*>& faces) { m_faces = faces; }
	bool Load();
private:
	std::vector<const char*> m_faces;
};
