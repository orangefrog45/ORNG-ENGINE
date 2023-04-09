#include "Terrain.h"


Terrain::Terrain(unsigned int seed, int width, glm::vec3 center_pos, int resolution, float height_scale, float sampling_density) : m_seed(seed),
m_width(width), m_center_pos(center_pos), m_resolution(resolution), m_height_scale(height_scale), m_sampling_density(sampling_density)
{
	m_quadtree = std::make_unique<TerrainQuadtree>(width, height_scale, sampling_density, seed, center_pos, resolution, &m_loader);
};


void Terrain::UpdateTerrainQuadtree() {
	m_quadtree->Update();
	m_loader.ProcessChunkLoads();
}


void Terrain::Init(Camera& camera) {
	m_quadtree->Init(camera);
	m_texture_array.AddTexture("./res/textures/terrain/snow_02_8k/textures/snow_02_diff_8k.jpg");
	m_texture_array.AddTexture("./res/textures/terrain/snow_02_8k/textures/snow_02_nor_gl_8k.jpg");
	m_texture_array.AddTexture("./res/textures/terrain/rocks_ground_05_8k/textures/rocks_ground_05_diff_8k.jpg");
	m_texture_array.AddTexture("./res/textures/terrain/rocks_ground_05_8k/textures/rocks_ground_05_nor_gl_8k.jpg");
	m_texture_array.AddTexture("./res/textures/terrain/forest_leaves_03_8k/textures/forest_leaves_03_diff_8k.jpg");
	m_texture_array.AddTexture("./res/textures/terrain/forest_leaves_03_8k/textures/forest_leaves_03_nor_gl_8k.jpg");
	m_texture_array.Load();
	m_terrain_top_mat.specular_color = glm::vec3(1.f);
	m_terrain_top_mat.diffuse_color = glm::vec3(1);

}
