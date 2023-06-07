#pragma once
#include "rendering/Material.h"
#include "terrain/TerrainQuadtree.h"
#include "terrain/ChunkLoader.h"
#include "rendering/Textures.h"

namespace ORNG {

	class Camera;

	class Terrain {
	public:
		friend class EditorLayer;
		friend class SceneRenderer;
		Terrain() = default;
		void Init(Camera& camera, unsigned int material_id);
		void UpdateTerrainQuadtree();
		void ResetTerrainQuadtree(Camera& camera);

		Texture2DArray m_diffuse_texture_array = Texture2DArray("Terrain diffuse");
		Texture2DArray m_normal_texture_array = Texture2DArray("Terrain normals");
	private:
		ChunkLoader m_loader;
		std::unique_ptr<TerrainQuadtree> m_quadtree = nullptr;

		unsigned int m_material_id;
		glm::vec3 m_center_pos = glm::vec3(0.f, 0.f, 0.f);
		unsigned int m_width = 10000;
		unsigned int m_resolution = 16;
		unsigned int m_seed = 123;
		float m_height_scale = 3.5;
	};

}