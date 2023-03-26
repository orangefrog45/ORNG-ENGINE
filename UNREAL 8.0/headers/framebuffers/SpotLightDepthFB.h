#pragma once
class SpotLightDepthFB {
public:
	void Init();
	void BindForDraw();
	unsigned int GetDepthMap() const { return m_spot_depth_array; };
	void SetDepthTexLayer(unsigned int layer);
private:
	const unsigned int m_spot_light_shadow_width = 1024;
	const unsigned int m_spot_light_shadow_height = 1024;
	unsigned int m_spot_depth_array = 0;
	unsigned int m_fbo = 0;
};
