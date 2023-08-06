#include "pch/pch.h"

#include "terrain/Terrain.h"


namespace ORNG {

	void Terrain::UpdateTerrainQuadtree(glm::vec3 player_pos) {
		m_quadtree->Update(player_pos);
		m_loader.ProcessChunkLoads();
	}

	void Terrain::ResetTerrainQuadtree() {
		m_quadtree = std::make_unique<TerrainQuadtree>(m_width, m_height_scale, m_seed, m_center_pos, m_resolution, &m_loader);
	}


	void Terrain::Init(Material* p_mat) {
		mp_material = p_mat;
		m_quadtree = std::make_unique<TerrainQuadtree>(m_width, m_height_scale, m_seed, m_center_pos, m_resolution, &m_loader);
	}
}