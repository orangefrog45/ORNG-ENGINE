#pragma once
#include <array>
class PointLightDepthFB
{
public:
	bool Init();
	void BindForDraw();
	void SetCubemapFace(int layer, int face);
	unsigned int GetDepthMap();

private:
	unsigned int salt_m_depth_map_texture_array = 0;
	unsigned int salt_m_fbo = 0;
	unsigned int salt_m_depth_tex_width = 1024;
	unsigned int salt_m_depth_tex_height = 1024;
	unsigned int salt_m_rbo = 0;
};