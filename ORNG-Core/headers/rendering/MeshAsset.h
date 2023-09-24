#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "rendering/Material.h"
#include "components/BoundingVolume.h"
#include "VAO.h"
#include "util/UUID.h"

#define ORNG_MAX_MESH_INDICES 50'000'000


class aiScene;
class aiMesh;


namespace ORNG {

	class TransformComponent;

	class MeshAsset : public Asset {
	public:
		friend class Renderer;
		friend class EditorLayer;
		friend class MeshInstanceGroup;
		friend class SceneRenderer;
		friend class MeshInstancingSystem;
		friend class SceneSerializer;
		friend class CodedAssets;
		friend class AssetManager;


		MeshAsset() = delete;
		MeshAsset(const std::string& filename) : Asset(filename) {};
		MeshAsset(const std::string& filename, uint64_t t_uuid) : Asset(filename) { uuid = UUID(t_uuid); };
		MeshAsset(const MeshAsset& other) = default;
		~MeshAsset() = default;

		bool LoadMeshData();

		bool GetLoadStatus() const { return m_is_loaded; };

		unsigned int GetIndicesCount() const { return num_indices; }

		const AABB& GetAABB() const { return m_aabb; }

		const MeshVAO& GetVAO() const { return m_vao; }


		template<typename S>
		void serialize(S& s) {
			s.object(m_vao);
			s.object(m_aabb);
			s.value4b((uint32_t)m_submeshes.size());
			for (auto& entry : m_submeshes) {
				s.object(entry);
			}
			s.value1b((uint8_t)num_materials);
			s.object(uuid);
		}

	private:

		// Callback for when this mesh has a VAO created for it and its materials set up by the AssetManager class
		// This is split from "LoadMeshData" so vertex data can be loaded asynchronously, cannot create the VAO asynchronously due to opengl contexts
		void OnLoadIntoGL();


		bool InitFromScene(const aiScene* pScene);

		void ReserveSpace(unsigned int NumVertices, unsigned int NumIndices);

		void InitAllMeshes(const aiScene* pScene);

		void InitSingleMesh(const aiMesh* paiMesh);

		void CountVerticesAndIndices(const aiScene* pScene, unsigned int& NumVertices, unsigned int& NumIndices);

		void PopulateBuffers();

		MeshVAO m_vao;

		AABB m_aabb;

		Assimp::Importer m_importer;

		unsigned int num_indices = 0;
		uint8_t num_materials = 0;

		bool m_is_loaded = false;

		const aiScene* p_scene = nullptr;


#define INVALID_MATERIAL 0xFFFFFFFF

		struct MeshEntry {
			MeshEntry() : num_indices(0), base_vertex(0), base_index(0), material_index(INVALID_MATERIAL) {};

			unsigned int num_indices;
			unsigned int base_vertex;
			unsigned int base_index;
			unsigned int material_index;
		};

		std::vector<MeshEntry> m_submeshes;

	};
}