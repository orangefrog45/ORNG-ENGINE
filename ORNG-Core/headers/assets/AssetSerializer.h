#pragma once
#include "../rendering/Textures.h"
#include "SoundAsset.h"
#include "../rendering/Material.h"
#include "../rendering/MeshAsset.h"
#include <bitsery/include/bitsery/serializer.h>
#include <bitsery/include/bitsery/deserializer.h>
#include <bitsery/include/bitsery/adapter/stream.h>
#include <bitsery/include/bitsery/traits/vector.h>

struct GLFWwindow;

namespace bitsery {
	using namespace ORNG;
	template <typename S>
	void serialize(S& s, glm::vec3& o) {
		s.value4b(o.x);
		s.value4b(o.y);
		s.value4b(o.z);
	}

	template <typename S>
	void serialize(S& s, glm::vec4& o) {
		s.value4b(o.x);
		s.value4b(o.y);
		s.value4b(o.z);
		s.value4b(o.w);
	}

	template<typename S>
	void serialize(S& s, glm::vec2& v) {
		s.value4b(v.x);
		s.value4b(v.y);
	}

	template <typename S>
	void serialize(S& s, VertexData3D& o) {
		s.container4b(o.positions, ORNG_MAX_MESH_INDICES);
		s.container4b(o.normals, ORNG_MAX_MESH_INDICES);
		s.container4b(o.tangents, ORNG_MAX_MESH_INDICES);
		s.container4b(o.tex_coords, ORNG_MAX_MESH_INDICES);
		s.container4b(o.indices, ORNG_MAX_MESH_INDICES);
	}

	template <typename S>
	void serialize(S& s, AABB& o) {
		s.object(o.extents);
		s.object(o.center);
	}

	template <typename S>
	void serialize(S& s, MeshEntry& o) {
		s.value4b(o.base_index);
		s.value4b(o.base_vertex);
		s.value4b(o.material_index);
		s.value4b(o.num_indices);
	}

	template<typename S>
	void serialize(S& s, MeshVAO& o) {
		s.object(o.vertex_data);
	}
}

namespace ORNG {
	class AssetManager;
	using BufferDeserializer = bitsery::Deserializer<bitsery::InputBufferAdapter<std::vector<std::byte>>>;
	using BufferSerializer = bitsery::Serializer<bitsery::OutputBufferAdapter<std::vector<std::byte>>>;

	class AssetSerializer {
	public:
		explicit AssetSerializer(AssetManager& manager) : m_manager(manager) {};

		void Init();

		void IStallUntilMeshesLoaded();
		void ProcessAssetQueues();

		// Creates a binary serialized package of all assets loaded into manager
		void CreateBinaryAssetPackage(const std::string& output_path);

		// Deserializes a binary serialized package and loads all assets into manager
		void DeserializeAssetsFromBinaryPackage(const std::string& package_path);

		bool TryFetchRawTextureData(Texture2D& tex, std::vector<std::byte>& output);

		bool TryFetchRawSoundData(SoundAsset& sound, std::vector<std::byte>& output);

		// Serializes all assets in manager to "output_path/res", output_path is typically a project directory
		void SerializeAssets(const std::string& output_path);

		// Adds asset to a loading queue and loads it asynchronously
		void LoadMeshAsset(MeshAsset* p_asset, const std::string& raw_mesh_filepath);

		// Adds asset to a loading queue and loads it asynchronously
		void LoadTexture2D(Texture2D* p_tex);

		void LoadAssetsFromProjectPath(const std::string& project_dir);

		void LoadMeshAssetIntoGL(MeshAsset* p_asset);

		void LoadAsset(const std::string& rel_path);
		void LoadTexture2DAssetFromFile(const std::string& rel_path);
		void LoadMeshAssetFromFile(const std::string& rel_path);
		void LoadAudioAssetFromFile(const std::string& rel_path);
		void LoadMaterialAssetFromFile(const std::string& rel_path);
		void LoadPrefabAssetFromFile(const std::string& rel_path);
		void LoadScriptAssetFromFile(const std::string& rel_path);
		void LoadSceneAssetFromFile(const std::string& rel_path);

		void SerializeSceneAsset(class SceneAsset& scene_asset, BufferSerializer& ser);
		template<typename SerializerType>
		void SerializeTexture2D(Texture2D& tex, SerializerType& ser, std::byte* p_data = nullptr, size_t data_size = 0) {
			std::vector<std::byte> texture_data;
			if (p_data) {
				texture_data.resize(data_size);
				memcpy(texture_data.data(), p_data, data_size);
			}
			else {
				TryFetchRawTextureData(tex, texture_data);
			}

			ser.object(tex.m_spec);
			ser.object(tex.uuid);
			ser.container1b(texture_data, UINT64_MAX);
		}

		template<typename SerializerType>
		void SerializeSoundAsset(SoundAsset& sound, SerializerType& ser) {
			std::vector<std::byte> sound_data;
			TryFetchRawSoundData(sound, sound_data);

			ser.object(sound.uuid);
			ser.container1b(sound_data, UINT64_MAX);
		}

		template<typename S>
		static void DeserializeTexture2D(Texture2D& tex, std::vector<std::byte>& raw_data, S& des) {
			des.object(tex.m_spec);
			des.object(tex.uuid);
			des.container1b(raw_data, UINT64_MAX);

			tex.SetSpec(tex.m_spec); // Has to be called for texture to properly update
		}

		template<typename S>
		static void DeserializeSoundAsset(SoundAsset& sound, std::vector<std::byte>& raw_data, S& des) {
			des.object(sound.uuid);
			des.container1b(raw_data, UINT64_MAX);
		}

		template<typename S>
		static void DeserializeMeshAsset(MeshAsset& mesh, S& des) {
			des.object(mesh.m_vao);
			des.object(mesh.m_aabb);
			uint32_t size;
			des.value4b(size);
			mesh.m_submeshes.resize(size);
			for (int i = 0; i < size; i++) {
				des.object(mesh.m_submeshes[i]);
			}
			des.value4b(mesh.m_num_materials);
			des.object(mesh.uuid);
			des.container8b(mesh.m_material_uuids, 10000);
		}

		void DeserializeMaterialAsset(Material& data, BufferDeserializer& des);

		template<std::derived_from<Asset> T>
		void SerializeAssetToBinaryFile(T& asset, const std::string& filepath) {
			/* Use a different temporary filepath for the serialized output as the previous binary file needs to be
			opened and have data transferred. Previous file is overwritten at end of this function. */
			std::string temp_filepath = filepath + ".TEMP";

			auto dir = std::filesystem::path{filepath}.parent_path();
			if (!FileExists(dir.generic_string())) std::filesystem::create_directories(dir);

			std::ofstream s{ temp_filepath, s.binary | s.trunc | s.out };
			if (!s.is_open()) {
				ORNG_CORE_ERROR("Binary serialization error: Cannot open {0} for writing", filepath);
				return;
			}

			bitsery::Serializer<bitsery::OutputStreamAdapter> ser{ s };

			if constexpr (std::is_same_v<T, Texture2D>) {
				SerializeTexture2D(asset, ser);
			}
			else if constexpr (std::is_same_v<T, SoundAsset>) {
				SerializeSoundAsset(asset, ser);
			}
			else {
				ser.object(asset);
			}

			s.close();
			std::filesystem::rename(temp_filepath, filepath);
		}

		template <std::derived_from<Asset> T>
		void DeserializeAssetBinary(const std::string& filepath, T& data, std::any args = 0) {
			std::vector<std::byte> buf;
			ReadBinaryFile(filepath, buf);
			BufferDeserializer des{buf.begin(), buf.end()};

			if constexpr (std::is_same_v<T, MeshAsset>) {
				DeserializeMeshAsset(data, des);
			}
			else if constexpr (std::is_same_v<T, Material>) {
				DeserializeMaterialAsset(data, des);
			}
			else if constexpr (std::is_same_v<T, Texture2D>) {
				DeserializeTexture2D(data, *std::any_cast<std::vector<std::byte>*>(args), des);
			}
			else if constexpr (std::is_same_v<T, SoundAsset>) {
				DeserializeSoundAsset(data, *std::any_cast<std::vector<std::byte>*>(args), des);
			}
			else {
				des.object(data);
			}
		}

		struct MeshAssets {
			MeshAsset* p_mesh = nullptr;
			std::vector<Texture2D*> textures;
			std::vector<Material*> materials;
		};

		private:
			// Creates a mesh asset, texture assets and material assets if any exist
			MeshAssets CreateAssetsFromMeshData(MeshAsset* p_mesh, MeshLoadResult& result);

			AssetManager& m_manager;
			std::vector<std::future<std::pair<std::optional<MeshLoadResult>, MeshAsset*>>> m_mesh_loading_queue;
			std::vector<std::future<void>> m_texture_loading_queue;

			// Used for texture loading
			GLFWwindow* mp_loading_context = nullptr;
	};
}