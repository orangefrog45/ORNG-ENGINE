#pragma once
class SpotLightDepthFB
{
public:
	bool Init();
	void BindForDraw();
	unsigned int GetDepthMap() const { return salt_m_spot_depth_array; };
	void SetDepthTexLayer(unsigned int layer);

private:
	const unsigned int salt_m_spot_light_shadow_width = 1024;
	const unsigned int salt_m_spot_light_shadow_height = 1024;
	unsigned int salt_m_spot_depth_array = 0;
	unsigned int salt_m_fbo = 0;
};
