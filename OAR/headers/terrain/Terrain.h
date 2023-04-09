#pragma once
#include "Material.h"
#include "TerrainQuadtree.h"
#include "ChunkLoader.h"
#include "Texture2DArray.h"

class Camera;

class Terrain {
public:
	friend class EditorLayer;
	friend class Renderer;
	Terrain() { m_loader.SetChunkSettings(m_seed, m_height_scale, m_sampling_density, m_width);  m_quadtree = std::make_unique<TerrainQuadtree>(m_width, m_height_scale, m_sampling_density, m_seed, m_center_pos, m_resolution, &m_loader); };
	Terrain(unsigned int seed, int width, glm::vec3 center_pos, int resolution, float height_scale, float sampling_density);
	void Init(Camera& camera);
	void UpdateTerrain(unsigned int seed, int width, glm::vec3 center_pos, int resolution, float height_scale, float sampling_density);
	void UpdateTerrainQuadtree();
private:
	ChunkLoader m_loader;
	Material m_terrain_top_mat;
	std::unique_ptr<TerrainQuadtree> m_quadtree = nullptr;
	Texture2DArray m_texture_array = Texture2DArray(8192, 8192);

	glm::vec3 m_center_pos = glm::vec3(0.f, 1000.f, 0.f);
	unsigned int m_width = 10000;
	unsigned int m_resolution = 16;
	float m_sampling_density = 5;
	unsigned int m_seed = 123;
	float m_height_scale = 3.5;
};