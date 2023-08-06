#pragma once
#include "TerrainGenerator.h"
#include "rendering/VAO.h"
#include "components/BoundingVolume.h"

namespace ORNG {

	class TerrainChunk {
		friend class SceneRenderer;
		friend class TerrainQuadtree;
		friend class ChunkLoader;
	public:
		TerrainChunk(glm::vec3 bot_left_coord, unsigned int resolution, unsigned int width, unsigned int seed, float height_scale) :
			m_bot_left_coord(bot_left_coord), m_resolution(resolution), m_width(width), m_seed(seed), m_height_scale(height_scale) {};

		void LoadGLData();
	private:
		/*Key for accessing chunk loader loading buffer, required for deletion if chunk not required anymore*/
		unsigned int m_chunk_key;

		AABB m_bounding_box;
		VAO m_vao;
		TerrainGenerator::TerrainData data;

		glm::vec3 m_bot_left_coord;
		unsigned int m_resolution;
		float m_height_scale;
		unsigned int m_width;
		unsigned int m_seed;
		bool m_is_initialized = false;
		bool m_data_is_loaded = false;
		bool is_loading = false;
	};
}