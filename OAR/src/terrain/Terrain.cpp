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


	void Terrain::Init(unsigned int material_id) {
		m_quadtree = std::make_unique<TerrainQuadtree>(m_width, m_height_scale, m_seed, m_center_pos, m_resolution, &m_loader);

		m_material_id = material_id;

		Texture2DArraySpec spec;
		spec.internal_format = GL_SRGB8_ALPHA8;
		spec.format = GL_RGBA;
		spec.width = 2048;
		spec.height = 2048;
		spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
		spec.mag_filter = GL_LINEAR;
		spec.generate_mipmaps = true;
		spec.filepaths = { "./res/textures/terrain/coast_sand_rocks_02_diff_2k.jpg", "./res/textures/terrain/aerial_rocks_02_diff_2k.jpg" };
		spec.storage_type = GL_UNSIGNED_BYTE;

		Texture2DArraySpec normal_spec;
		normal_spec.internal_format = GL_RGBA8;
		normal_spec.format = GL_RGBA;
		normal_spec.width = 2048;
		normal_spec.height = 2048;
		normal_spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
		normal_spec.mag_filter = GL_LINEAR;
		normal_spec.generate_mipmaps = true;
		normal_spec.filepaths = { "./res/textures/terrain/coast_sand_rocks_02_nor_gl_2k.jpg", "./res/textures/terrain/aerial_rocks_02_nor_gl_2k.jpg" };
		normal_spec.storage_type = GL_UNSIGNED_BYTE;

		m_diffuse_texture_array.SetSpec(spec);
		m_diffuse_texture_array.LoadFromFile();
		m_normal_texture_array.SetSpec(normal_spec);
		m_normal_texture_array.LoadFromFile();

	}
}