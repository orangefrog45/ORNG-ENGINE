#pragma once
class MainViewFramebuffer {
public:
	MainViewFramebuffer(unsigned int t_window_width, unsigned int t_window_height) : m_window_width(t_window_width), m_window_height(t_window_height) {}
	~MainViewFramebuffer();
	unsigned int GetTexture() { return m_render_texture; };
	void Init();
	void Bind();
	void Unbind();
private:
	unsigned int m_window_width;
	unsigned int m_window_height;
	unsigned int m_render_texture;
	unsigned int m_fbo;
	unsigned int m_rbo;
};