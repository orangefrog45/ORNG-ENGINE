#pragma once

class DirLightDepthFB {
public:
	bool Init();
	void BindForDraw();
	unsigned int GetDepthMap();
private:
	const unsigned int m_dir_light_shadow_width = 4096;
	const unsigned int m_dir_light_shadow_height = 4096;
	unsigned int dir_depth_tex;
	unsigned int m_fbo;

};