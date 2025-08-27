#pragma once

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "rendering/Material.h"
#include "components/BoundingVolume.h"
#include "VAO.h"
#include "util/UUID.h"

constexpr unsigned ORNG_MAX_MESH_INDICES = 50'000'000;

struct aiScene;
struct aiMesh;

namespace ORNG {
	struct LoadedMeshTexture {
		Texture2D* p_tex = nullptr;
		std::vector<std::byte> data;
		Texture2DSpec spec;
		bool is_decompressed;
	};

	inline static constexpr unsigned INVALID_MATERIAL = 0xFFFFFFFF;
	struct MeshEntry {
		MeshEntry() : num_indices(0), base_vertex(0), base_index(0), material_index(INVALID_MATERIAL) {};

		unsigned int num_indices;
		unsigned int base_vertex;
		unsigned int base_index;
		unsigned int material_index;
	};

	// Textures and materials here are heap-allocated and need to be freed later
	struct MeshLoadResult {
		void Free() {
			for (auto& tex : textures) {
				delete tex.p_tex;
			}
			textures.clear();

			for (auto* p_mat : materials) {
				delete p_mat;
			}
			materials.clear();
		}

		std::string original_file_path;
		VertexData3D vertex_data;
		std::vector<LoadedMeshTexture> textures;
		std::vector<Material*> materials;
		// Contains indices into the textures array
		std::unordered_map<std::string, unsigned> texture_name_lookup;
		std::vector<MeshEntry> submeshes;
		AABB aabb;

		unsigned num_vertices = 0;
		unsigned num_indices = 0;
	};

	class MeshAsset : public Asset {
	public:
		friend class Renderer;
		friend class EditorLayer;
		friend class MeshInstanceGroup;
		friend class SceneRenderer;
		friend class MeshInstancingSystem;
		friend class SceneSerializer;
		friend class AssetManager;
		friend class AssetSerializer;

		MeshAsset() = delete;
		MeshAsset(const std::string& filename) : Asset(filename) {};
		MeshAsset(const std::string& filename, uint64_t t_uuid) : Asset(filename) { uuid = UUID(t_uuid); };
		MeshAsset(const MeshAsset& other) = default;
		virtual ~MeshAsset() = default;

		static std::optional<MeshLoadResult> LoadMeshDataFromFile(const std::string& raw_mesh_filepath);

		// 'result.vertex_data' is moved during this function call, do not use it afterwards
		void SetMeshData(MeshLoadResult& result);

		bool GetLoadStatus() const { return m_is_loaded; };

		unsigned int GetIndicesCount() const { return m_num_indices; }

		const AABB& GetAABB() const { return m_aabb; }

		const MeshVAO& GetVAO() const { return m_vao; }

		void ClearCPU_VertexData() {
			glFinish();
			m_vao.vertex_data.positions.clear();
			m_vao.vertex_data.normals.clear();
			m_vao.vertex_data.tangents.clear();
			m_vao.vertex_data.tex_coords.clear();
			m_vao.vertex_data.indices.clear();
		}

		unsigned GetNbMaterials() {
			return m_num_materials;
		}

		template<typename S>
		void serialize(S& s) {
			s.object(m_vao);
			s.object(m_aabb);
			s.value4b((uint32_t)m_submeshes.size());
			for (auto& entry : m_submeshes) {
				s.object(entry);
			}
			s.value4b(m_num_materials);
			s.object(uuid);
			s.container8b(m_material_uuids, 10000);
		}

		const std::vector<MeshEntry>& GetSubmeshes() {
			return m_submeshes;
		}

		[[nodiscard]] const std::vector<uint64_t>& GetMaterialUUIDs() const noexcept {
			return m_material_uuids;
		}

	private:
		static bool ProcessEmbeddedTexture(Texture2D* p_tex, const aiTexture* p_ai_tex);

		static bool InitFromScene(const aiScene* p_scene, MeshLoadResult& result);

		static void InitAllMeshes(const aiScene* p_scene, MeshLoadResult& result);

		static void InitSingleMesh(const aiMesh* p_ai_mesh, unsigned current_idx, unsigned current_vertex, MeshLoadResult& result);

		static void CountVerticesAndIndices(const aiScene* p_scene, unsigned int& num_verts, unsigned int& num_indices, MeshLoadResult& result);

		static void LoadMaterialsAndTextures(MeshLoadResult& result, const std::string& dir, const aiScene* p_scene);

		static LoadedMeshTexture* CreateOrGetMaterialTexture(const std::string& dir, const aiTextureType& type, const aiMaterial* p_material, MeshLoadResult& result, const aiScene* p_scene);

		MeshVAO m_vao;

		AABB m_aabb;
		unsigned m_num_indices = 0;
		unsigned m_num_materials = 0;
		bool m_is_loaded = false;
		const aiScene* p_scene = nullptr;
		std::vector<MeshEntry> m_submeshes;

		// The UUIDs of the materials that got loaded in from this mesh
		// If these materials are deleted, the base material will be used instead
		std::vector<uint64_t> m_material_uuids;
	};
}