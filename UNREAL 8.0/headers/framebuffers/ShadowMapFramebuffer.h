#pragma once
class ShadowMapFramebuffer {
public:
	void Init();
	void BindForDraw();
	void Unbind();
	unsigned int GetDepthMapTexture();
private:
	const unsigned int m_shadow_width = 4096;
	const unsigned int m_shadow_height = 4096;
	unsigned int depth_map_fbo;
	unsigned int depth_map_texture;
};
