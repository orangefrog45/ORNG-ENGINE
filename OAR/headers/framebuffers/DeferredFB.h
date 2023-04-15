#pragma once
class DeferredFB
{
public:
	bool Init();
	void BindGeometryTexture();
	void BindForDraw();
	inline unsigned int GetGeometryTexture() const { return salt_m_geometry_texture; };

private:
	unsigned int salt_m_fbo = 0;
	unsigned int salt_m_rbo = 0;
	unsigned int salt_m_geometry_texture = 0;
};
