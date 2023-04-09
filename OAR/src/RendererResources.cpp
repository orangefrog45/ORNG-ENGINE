#include "RendererResources.h"

void RendererResources::BindTexture(int target, int texture, int tex_unit) {
	GLCall(glActiveTexture(tex_unit));
	GLCall(glBindTexture(target, texture));
}

void RendererResources::IInit() {
	m_missing_texture = std::make_unique<Texture2D>("./res/textures/missing_texture.jpeg");
	m_missing_texture->Load();
}