#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "rendering/Material.h"
#include "components/BoundingVolume.h"
#include "VAO.h"

namespace ORNG {

	class WorldTransform;

	class MeshAsset {
	public:
		friend class Scene;
		friend class Renderer;
		friend class EditorLayer;
		friend class MeshInstanceGroup;
		friend class SceneRenderer;

		MeshAsset() = default;
		MeshAsset(const std::string& filename) : m_filename(filename) {};
		MeshAsset(const MeshAsset& other) = default;
		~MeshAsset();

		bool LoadMeshData();


		std::string GetFilename() const { return m_filename; };

		bool GetLoadStatus() const { return is_loaded; };

		void SetIsShared(const bool val) { m_is_shared_in_instance_groups = val; }

		bool GetIsShared() const { return m_is_shared_in_instance_groups; }

		unsigned int GetIndicesCount() const { return num_indices; }

	private:
		AABB m_aabb;

		std::array<glm::vec3, 36> m_bounding_box_vertices;

		unsigned int num_indices = 0;

		/* is_shared_in_instance_groups, if true, causes GL world transform buffers to update once each frame for each different instance group using this mesh -
	(added for multiple shader support, slower performance) */
		bool m_is_shared_in_instance_groups = false;

		bool is_loaded = false;

		const aiScene* p_scene = nullptr;

		Assimp::Importer importer;

		bool InitFromScene(const aiScene* pScene, const std::string& filename);

		void ReserveSpace(unsigned int NumVertices, unsigned int NumIndices);

		void InitAllMeshes(const aiScene* pScene);

		void InitSingleMesh(const aiMesh* paiMesh);

		void CountVerticesAndIndices(const aiScene* pScene, unsigned int& NumVertices, unsigned int& NumIndices);

		void PopulateBuffers();

		std::string m_filename = "";

		VAO m_vao;


#define INVALID_MATERIAL 0xFFFFFFFF

		struct MeshEntry {
			MeshEntry() : num_indices(0), base_vertex(0), base_index(0), material_index(INVALID_MATERIAL) {};

			unsigned int num_indices;
			unsigned int base_vertex;
			unsigned int base_index;
			unsigned int material_index;
		};

		std::vector<MeshEntry> m_submeshes;
		std::vector<Material> m_original_materials;

		// Pointers to the material assets created in a scene with this asset
		std::vector<Material*> m_scene_materials;

	};
}