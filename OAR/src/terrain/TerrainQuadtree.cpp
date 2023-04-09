#include "TerrainQuadtree.h"
#include "Log.h"
#include "util/util.h"
#include "TerrainChunk.h"
#include "ChunkLoader.h"
#include "Camera.h"

void TerrainQuadtree::Subdivide() {
	if (m_subdivision_layer == m_max_subdivision_layer || m_is_subdivided) {
		OAR_CORE_ERROR("Max terrain quadtree subdivisions hit");
		return;
	}
	m_child_nodes.reserve(4);

	m_child_nodes.emplace_back(glm::vec3(m_center_pos.x + m_width * 0.25f, m_center_pos.y, m_center_pos.z - m_width * 0.25f), this); //TR
	m_child_nodes.emplace_back(glm::vec3(m_center_pos.x - m_width * 0.25f, m_center_pos.y, m_center_pos.z - m_width * 0.25f), this); //TL
	m_child_nodes.emplace_back(glm::vec3(m_center_pos.x + m_width * 0.25f, m_center_pos.y, m_center_pos.z + m_width * 0.25f), this); //BR
	m_child_nodes.emplace_back(glm::vec3(m_center_pos.x - m_width * 0.25f, m_center_pos.y, m_center_pos.z + m_width * 0.25f), this); //BL

	m_is_subdivided = true;

}

TerrainQuadtree::~TerrainQuadtree() {
	delete m_chunk;
}

TerrainQuadtree::TerrainQuadtree(unsigned int width, int height_scale, unsigned int sampling_density, unsigned int seed, glm::vec3 center_pos, unsigned int resolution, ChunkLoader* loader) :
	m_center_pos(center_pos), m_width(width), m_sampling_density(sampling_density), m_seed(seed), m_height_scale(height_scale), m_resolution(resolution), m_master_width(width),
	m_is_root_node(true), m_loader(loader)
{
	/* glm::log2(m_master_width / m_min_grid_width) == no. subdivisions before min grid width is hit */
	m_max_subdivision_layer = glm::clamp(static_cast<int>(glm::log2(static_cast<float>(m_master_width) / static_cast<float>(m_min_grid_width))), 5, 15);
	OAR_CORE_INFO("Subdivision layer : {0}", m_max_subdivision_layer);

	glm::vec3 bot_left_coord = glm::vec3(center_pos.x - m_width * 0.5f, center_pos.y, center_pos.z - m_width * 0.5f);
	m_chunk = new TerrainChunk(bot_left_coord, m_resolution, m_width, m_seed, m_height_scale, m_sampling_density);
	m_loader->RequestChunkLoad(*m_chunk);
};

TerrainQuadtree::TerrainQuadtree(glm::vec3 center_pos, TerrainQuadtree* parent)
{
	m_parent = parent;
	m_subdivision_layer = parent->m_subdivision_layer + 1;
	m_max_subdivision_layer = parent->m_max_subdivision_layer;
	m_seed = parent->m_seed;
	m_width = parent->m_width * 0.5f;
	m_loader = parent->m_loader;
	m_min_grid_width = parent->m_min_grid_width;

	m_sampling_density = parent->m_sampling_density;
	m_height_scale = parent->m_height_scale;

	m_lod_camera = parent->m_lod_camera;
	m_center_pos = center_pos;
	m_master_width = parent->m_master_width;

	if (m_subdivision_layer >= 0 && m_subdivision_layer <= m_max_subdivision_layer * 0.15) {
		m_resolution = 24.f;
	}
	else if (m_subdivision_layer >= m_max_subdivision_layer * 0.15 && m_subdivision_layer <= m_max_subdivision_layer * 0.3) {
		m_resolution = 12.f;
	}
	else if (m_subdivision_layer >= m_max_subdivision_layer * 0.3 && m_subdivision_layer <= m_max_subdivision_layer * 0.45) {
		m_resolution = 8.f;
	}
	else if (m_subdivision_layer >= m_max_subdivision_layer * 0.45 && m_subdivision_layer <= m_max_subdivision_layer * 0.6) {
		m_resolution = 6.f;
	}
	else if (m_subdivision_layer >= m_max_subdivision_layer * 0.6 && m_subdivision_layer <= m_max_subdivision_layer * 0.7) {
		m_resolution = 3.5f;
	}
	else if (m_subdivision_layer >= m_max_subdivision_layer * 0.7 && m_subdivision_layer <= m_max_subdivision_layer * 0.85) {
		m_resolution = 2.5f;
	}
	else if (m_subdivision_layer >= m_max_subdivision_layer * 0.85 && m_subdivision_layer < m_max_subdivision_layer) {
		m_resolution = 1.25f;
	}
	else if (m_subdivision_layer == m_max_subdivision_layer) {
		m_resolution = 0.75f;
	}
	OAR_CORE_INFO("Subdivision layer : {0}, Width: {1}", m_subdivision_layer, m_width);

	glm::vec3 bot_left_coord = glm::vec3(center_pos.x - m_width * 0.5, center_pos.y, center_pos.z - m_width * 0.5);
	m_chunk = new TerrainChunk(bot_left_coord, m_resolution, m_width, m_seed, m_height_scale, m_sampling_density);
	m_loader->RequestChunkLoad(*m_chunk);


};

void TerrainQuadtree::Init(Camera& camera) {
	m_lod_camera = &camera;
};


void TerrainQuadtree::QueryTree(std::vector<TerrainQuadtree*>& node_array, glm::vec3 t_center_pos, int t_boundary) {
	if (glm::length(glm::vec2(m_center_pos.x, m_center_pos.z) - glm::vec2(t_center_pos.x, t_center_pos.z)) < t_boundary + glm::sqrt(2 * m_width * m_width)) {
		if (m_is_subdivided) {
			for (auto& node : m_child_nodes) {
				node.QueryTree(node_array, t_center_pos, t_boundary);
			}
		}
		else {
			node_array.push_back(this);
			return;
		}
	}
	return;
};

void TerrainQuadtree::QueryChunks(std::vector<TerrainQuadtree*>& node_array, glm::vec3 t_center_pos, int t_boundary) {
	if (glm::length(glm::vec2(m_center_pos.x, m_center_pos.z) - glm::vec2(t_center_pos.x, t_center_pos.z)) < t_boundary + glm::sqrt(2 * m_width * m_width)) {
		if (m_is_subdivided) {
			for (auto& node : m_child_nodes) {
				node.QueryChunks(node_array, t_center_pos, t_boundary);
			}
		}
		else if (m_chunk->m_is_initialized) {
			node_array.push_back(this);
		}
		else {
			/* This prevents gaps appearing in terrain when leaf node chunks aren't ready to render yet */
			TerrainQuadtree* found_chunk = FindParentWithChunk();
			if (found_chunk) node_array.push_back(found_chunk);
		}
		return;
	}
	return;
};

TerrainQuadtree* TerrainQuadtree::FindParentWithChunk() {
	if (!this->m_is_root_node) {
		if (this->GetParent().m_chunk->m_is_initialized) {
			return &this->GetParent();
		}
		else {
			return this->GetParent().FindParentWithChunk();
		}
	}
	else {
		return nullptr;
	}
}

void TerrainQuadtree::Update() {

	auto pos = m_lod_camera->GetPos();

	const int distance = glm::ceil(glm::length(glm::vec2(m_center_pos.x, m_center_pos.z) - glm::vec2(pos.x, pos.z))) - glm::sqrt(2 * m_width * m_width);
	int subdivision_count = m_max_subdivision_layer - glm::min((distance) / m_max_subdivision_layer, m_max_subdivision_layer);

	if (subdivision_count < m_subdivision_layer) {
		Unsubdivide();
	}
	else if (subdivision_count > m_subdivision_layer) {
		if (m_is_subdivided) {
			for (auto& node : m_child_nodes) {
				node.Update();
			}
		}
		else if (m_subdivision_layer != m_max_subdivision_layer) {
			Subdivide();
		}
	}

}

void TerrainQuadtree::Unsubdivide() {
	for (auto& node : m_child_nodes) {
		node.Unsubdivide();
		m_loader->CancelChunkLoad(node.m_chunk->m_chunk_key);
	}
	m_child_nodes.clear();

	m_is_subdivided = false;
}

bool TerrainQuadtree::CheckIntersection(glm::vec3 position) {

	if (position.x > m_center_pos.x + m_width ||
		position.x < m_center_pos.x - m_width ||
		position.z > m_center_pos.z - m_width ||
		position.z < m_center_pos.z + m_width
		) {
		return false;
	}
	else {
		return true;
	}
}

TerrainQuadtree& TerrainQuadtree::DoBoundaryTest(glm::vec3 position) {

	if (!m_is_subdivided) return *this;

	for (auto& node : m_child_nodes) {
		if (node.CheckIntersection(position)) {
			return node.DoBoundaryTest(position);
		}
	}

}
