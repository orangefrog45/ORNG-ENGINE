#pragma once

class DirLightDepthFB
{
public:
	bool Init();
	void BindForDraw();
	unsigned int GetDepthMap();

private:
	const unsigned int salt_m_dir_light_shadow_width = 4096;
	const unsigned int salt_m_dir_light_shadow_height = 4096;
	unsigned int salt_dir_depth_tex;
	unsigned int salt_m_fbo;
};