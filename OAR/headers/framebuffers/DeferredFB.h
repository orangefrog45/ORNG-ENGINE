#pragma once
class DeferredFB {
public:
	bool Init();
	void BindGeometryTexture();
	void BindForDraw();
	inline unsigned int GetGeometryTexture() const { return m_geometry_texture; };
private:
	unsigned int m_fbo = 0;
	unsigned int m_rbo = 0;
	unsigned int m_geometry_texture = 0;
};
