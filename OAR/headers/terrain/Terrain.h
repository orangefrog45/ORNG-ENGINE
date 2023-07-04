#pragma once
#include "terrain/TerrainQuadtree.h"
#include "terrain/ChunkLoader.h"

namespace ORNG {


	class Terrain {
	public:
		friend class EditorLayer;
		friend class SceneRenderer;
		Terrain() = default;
		void Init(uint64_t material_id);
		void UpdateTerrainQuadtree(glm::vec3 player_pos);
		void ResetTerrainQuadtree();

	private:
		ChunkLoader m_loader;
		std::unique_ptr<TerrainQuadtree> m_quadtree = nullptr;

		glm::vec3 m_center_pos = glm::vec3(0.f, 0.f, 0.f);
		uint64_t m_material_id;
		unsigned int m_width = 0;
		unsigned int m_resolution = 16;
		unsigned int m_seed = 123;
		float m_height_scale = 1.5f;
	};

}