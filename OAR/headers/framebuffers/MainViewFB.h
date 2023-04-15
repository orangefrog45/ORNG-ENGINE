#pragma once
class MainViewFB
{
public:
	MainViewFB() = default;
	~MainViewFB();
	unsigned int GetTexture() { return salt_m_render_texture; };
	unsigned int GetDepthTexture() { return salt_m_depth_texture; };
	bool Init();
	void Bind();

private:
	unsigned int salt_m_render_texture;
	unsigned int salt_m_depth_texture;
	unsigned int salt_m_fbo;
};