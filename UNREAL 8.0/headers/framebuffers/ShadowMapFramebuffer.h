#pragma once
class ShadowMapFramebuffer {
public:
	void Init();
	void BindForDraw();
	void Unbind();
	unsigned int GetDepthMapTexture();
private:
	unsigned int depth_map_fbo;
	unsigned int depth_map_texture;
};
