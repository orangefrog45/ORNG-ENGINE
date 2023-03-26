#pragma once
#include "TerrainGenerator.h"
#include "Texture.h"
#include "Material.h"

class Terrain {
public:
	friend class Renderer;
	Terrain();
	void Init();
	void UpdateTerrain(int width_x, int width_z, glm::fvec3 pos, float resolution);
	inline unsigned int GetVao() const { return m_vao; }
private:
	Material m_terrain_top_mat;
	unsigned int m_vao = 0;
	unsigned int m_position_buffer = 0;
	unsigned int m_transform_buffer = 0;
	unsigned int m_normal_buffer = 0;
	unsigned int m_tex_coord_buffer = 0;
	int m_x_width = 0;
	int m_z_width = 0;
	int m_resolution = 0;
	TerrainGenerator::TerrainData m_terrain_data;
	std::vector<glm::fmat4> m_transforms;
};