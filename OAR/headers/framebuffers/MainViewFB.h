#pragma once
class MainViewFB {
public:
	MainViewFB() = default;
	~MainViewFB();
	unsigned int GetTexture() { return m_render_texture; };
	unsigned int GetDepthTexture() { return m_depth_texture; };
	bool Init();
	void Bind();
private:
	unsigned int m_render_texture;
	unsigned int m_depth_texture;
	unsigned int m_fbo;
};