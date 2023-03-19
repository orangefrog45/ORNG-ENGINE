#pragma once
class SpotLightDepthFB {
public:
	void Init();
	void BindForDraw();
	unsigned int GetDepthMap() const { return m_spot_depth_array; };
	void SetDepthTexLayer(unsigned int layer);
private:
	const unsigned int m_spot_light_shadow_width = 2048;
	const unsigned int m_spot_light_shadow_height = 2048;
	unsigned int m_spot_depth_array;
	unsigned int m_fbo;
};
