#pragma once
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include <vector>
#include "TerrainGenerator.h"

class TerrainChunk {
	friend class Renderer;
	friend class TerrainQuadtree;
	friend class ChunkLoader;
public:
	TerrainChunk(glm::vec3 bot_left_coord, float resolution, unsigned int width, unsigned int seed, float height_scale, float sampling_density) :
		m_bot_left_coord(bot_left_coord), m_resolution(resolution), m_width(width), m_seed(seed), m_height_scale(height_scale), m_sampling_density(sampling_density) {};
	void Init();
	~TerrainChunk();
private:
	TerrainGenerator::TerrainData m_terrain_data;
	/*Key for accessing chunk loader loading buffer, required for deletion if chunk not required anymore*/
	unsigned int m_chunk_key;


	unsigned int m_vao;
	unsigned int m_position_buffer;
	unsigned int m_tex_coord_buffer;
	unsigned int m_normal_buffer;
	unsigned int m_tangent_buffer;
	unsigned int m_transform_buffer;
	unsigned int m_index_buffer;

	std::vector<glm::mat4> m_transforms;

	glm::vec3 m_bot_left_coord;
	float m_resolution;
	unsigned int m_width;
	unsigned int m_seed;
	float m_height_scale;
	unsigned int m_sampling_density;
	bool m_is_initialized = false;
	bool m_data_is_loaded = false;
	bool is_loading = false;
};