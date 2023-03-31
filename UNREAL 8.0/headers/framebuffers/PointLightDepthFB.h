#pragma once
#include <array>
class PointLightDepthFB {
public:
	bool Init();
	void BindForDraw();
	void SetCubemapFace(int layer, int face);
	unsigned int GetDepthMap();
private:
	unsigned int m_depth_map_texture_array = 0;
	unsigned int m_fbo = 0;
	unsigned int m_depth_tex_width = 1024;
	unsigned int m_depth_tex_height = 1024;
	unsigned int m_rbo = 0;
};