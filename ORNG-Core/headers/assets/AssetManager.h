#include "rendering/Material.h"
#include "events/Events.h"
#include "scripting/ScriptingEngine.h"
#include "rendering/Textures.h"
#include "rendering/MeshAsset.h"
#include "assets/SoundAsset.h"
#include "PhysXMaterialAsset.h"
#include <bitsery/bitsery.h>
#include <bitsery/traits/vector.h>
#include <bitsery/adapter/stream.h>
#include "bitsery/traits/string.h"
#include "yaml-cpp/node/node.h"

#define ORNG_BASE_MATERIAL_ID 0
#define ORNG_BASE_SOUND_ID 1
#define ORNG_BASE_TEX_ID 2
#define ORNG_BASE_MESH_ID 3
#define ORNG_BASE_SCRIPT_ID	4
#define ORNG_BASE_SPHERE_ID 5
#define ORNG_BASE_PHYSX_MATERIAL_ID 6
#define ORNG_BASE_QUAD_ID 7
#define ORNG_BASE_BRDF_LUT_ID 8
#define ORNG_NUM_BASE_ASSETS 9


struct GLFWwindow;
enum aiTextureType;
struct aiMaterial;




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
		s.object(o.max);
		s.object(o.min);
		s.object(o.center);
	}

	template <typename S>
	void serialize(S& s, MeshAsset::MeshEntry& o) {
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
	class Texture2D;
	class MeshAsset;
	struct Texture2DSpec;

	struct Prefab : public Asset {
		Prefab(const std::string& filepath) : Asset(filepath) {};
		// Yaml string that can be deserialized into entity
		std::string serialized_content;

		// Parsed version of "serialized_content"
		YAML::Node node;

		template<typename S>
		void serialize(S& s) {
			s.text1b(serialized_content, 10000);
			s.object(uuid);
		}
	};



	class AssetManager {
	public:
		friend class AssetManagerWindow;
		friend class EditorLayer;
		friend class SceneSerializer;
		static AssetManager& Get() {
			static AssetManager s_instance;
			return s_instance;
		}

		static void Init() { Get().I_Init(); }

		// Asset's memory will be managed by asset manager, provide a ptr to a heap-allocated object that will not be destroyed
		template<std::derived_from<Asset> T>
		static T* AddAsset(T* p_asset) {
			uint64_t uuid = p_asset->uuid();
			ASSERT(!Get().m_assets.contains(uuid));

			Get().m_assets[uuid] = static_cast<Asset*>(p_asset);
			HandleAssetAddition(p_asset);
			return p_asset;
		}

		template<std::derived_from<Asset> T>
		static std::vector<T*> GetView() {
			std::vector<T*> vec;
			for (auto [uuid, p_asset] : Get().m_assets) {
				if (T* p_typed_asset = dynamic_cast<T*>(p_asset); p_typed_asset &&
					uuid >= ORNG_NUM_BASE_ASSETS
					)
					vec.push_back(p_typed_asset);
			}

			return vec;
		}

		// Returns ptr to asset or nullptr if no valid asset was found
		template<std::derived_from<Asset> T>
		static T* GetAsset(uint64_t uuid) {
			if (Get().m_assets.contains(uuid)) {
				if (auto* p_asset = dynamic_cast<T*>(Get().m_assets[uuid]))
					return p_asset;
				else {
					ORNG_CORE_TRACE("GetAsset failed, asset with uuid '{0}' doesn't match type provided", uuid);
					return nullptr;
				}
			}
			else {
				ORNG_CORE_TRACE("GetAsset failed, no asset with uuid '{0}' found", uuid);
				return nullptr;
			}
		}

		// Returns ptr to asset or nullptr if no valid asset was found
		template<std::derived_from<Asset> T>
		static T* GetAsset(const std::string& filepath) {
			for (auto& [uuid, p_asset] : Get().m_assets) {
				if (p_asset->PathEqualTo(filepath)) {
					if (auto* p_typed_asset = dynamic_cast<T*>(p_asset))
						return p_typed_asset;
					else {
						ORNG_CORE_TRACE("GetAsset failed, asset with path '{0}' doesn't match type provided", filepath);
					}
				}
			}

			ORNG_CORE_TRACE("No valid asset with path '{0}' found", filepath);
			return nullptr;
		}

		static bool DeleteAsset(uint64_t uuid) {
			if (Get().m_assets.contains(uuid)) {
				HandleAssetDeletion(Get().m_assets[uuid]);
				delete Get().m_assets[uuid];
				Get().m_assets.erase(uuid);

				return true;
			}
			else {
				ORNG_CORE_TRACE("DeleteAsset failed, no asset with uuid '{0}' found", uuid);
				return false;
			}
		}

		template <std::derived_from<Asset> T>
		static bool DeleteAsset(T* p_asset) {
			for (auto [key, val] : Get().m_assets) {
				if (val == dynamic_cast<T*>(p_asset)) {
					HandleAssetDeletion(val);
					delete p_asset;
					Get().m_assets.erase(key);
					return true;
				}
			}

			return false;
		}

		// Clears all assets including base replacement ones
		static void OnShutdown() {
			Get().IOnShutdown();
		};


		static void HandleAssetDeletion(Asset* p_asset);
		static void HandleAssetAddition(Asset* p_asset);

		static void LoadMeshAsset(MeshAsset* p_asset);
		static void LoadTexture2D(Texture2D* p_tex);

		static void LoadAssetsFromProjectPath(const std::string& project_dir, bool precompiled_scripts);

		static void SerializeAssets() {
			Get().ISerializeAssets();
		}
#
		// Deletes all assets
		inline static void ClearAll() { Get().IClearAll(); };

		// Stalls program and waits for meshes to load - this will cause the program to freeze
		inline static void StallUntilMeshesLoaded() { Get().IStallUntilMeshesLoaded(); }


		template<std::derived_from<Asset> T>
		static void SerializeAssetToBinaryFile(T& asset, const std::string& filepath) {
			/* Use a different temporary filepath for the serialized output as the previous binary file needs to be 
			opened and have data transferred. Previous file is overwritten at end of this function. */
			std::string temp_filepath = filepath + ".TEMP";

			std::ofstream s{ temp_filepath, s.binary | s.trunc | s.out};
			if (!s.is_open()) {
				ORNG_CORE_ERROR("Binary serialization error: Cannot open {0} for writing", filepath);
				return;
			}

			bitsery::Serializer<bitsery::OutputStreamAdapter> ser{ s };

			if constexpr (std::is_same_v<T, Texture2D>) {
				SerializeTexture2D(asset, filepath, ser);
			}
			else if constexpr (std::is_same_v<T, SoundAsset>) {
				SerializeSoundAsset(asset, filepath, ser);
			}
			else {
				ser.object(asset);
			}

			s.close();
			std::filesystem::rename(temp_filepath, filepath);
		}

		template <std::derived_from<Asset> T>
		static void DeserializeAssetBinary(const std::string& filepath, T& data, std::any args = 0) {
			std::ifstream s{ filepath, std::ios::binary };
			if (!s.is_open()) {
				ORNG_CORE_ERROR("Deserialization error: Cannot open {0} for reading", filepath);
				return;
			}

			// Use buffered stream adapter
			bitsery::Deserializer<bitsery::InputStreamAdapter> des{ s };

			if constexpr (std::is_same_v<T, MeshAsset>) {
				DeserializeMeshAsset(data, des);
			}
			else if constexpr (std::is_same_v<T, Material>) {
				DeserializeMaterialAsset(data, des);
			}
			else if constexpr (std::is_same_v<T, PhysXMaterialAsset>) {
				DeserializePhysxMaterialAsset(data, des);
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

			s.close();
		}

		static void CreateBinaryAssetPackage(const std::string& output_path) {
			Get().ICreateBinaryAssetPackage(output_path);
		}

		static void DeserializeAssetsFromBinaryPackage(const std::string& package_path) {
			Get().IDeserializeAssetsFromBinaryPackage(package_path);
		}

	private:
		void I_Init();

		void ICreateBinaryAssetPackage(const std::string& output_path);
		void IDeserializeAssetsFromBinaryPackage(const std::string& package_filepath);

		static void SerializeTexture2D(Texture2D& tex, const std::string& output_filepath, bitsery::Serializer<bitsery::OutputStreamAdapter>& ser);
		static void SerializeSoundAsset(SoundAsset& sound, const std::string& output_filepath, bitsery::Serializer<bitsery::OutputStreamAdapter>& ser);

		static void DeserializeTexture2D(Texture2D& tex, std::vector<std::byte>& raw_data, bitsery::Deserializer<bitsery::InputStreamAdapter>& des);
		static void DeserializeSoundAsset(SoundAsset& sound, std::vector<std::byte>& raw_data, bitsery::Deserializer<bitsery::InputStreamAdapter>& des);
		static void DeserializeMeshAsset(MeshAsset& mesh, bitsery::Deserializer<bitsery::InputStreamAdapter>& des);
		static void DeserializeMaterialAsset(Material& material, bitsery::Deserializer<bitsery::InputStreamAdapter>& des);
		static void DeserializePhysxMaterialAsset(PhysXMaterialAsset& material, bitsery::Deserializer<bitsery::InputStreamAdapter>& des);

		// Loads all base assets (assets the engine runtime requires) that require an external file, e.g the sphere mesh needs to be loaded from a binary file. These files are always present in the "res/core-res" folder of a project
		void LoadExternalBaseAssets(const std::string& project_dir);

		void IOnShutdown();
		void ISerializeAssets();

		static void OnTextureDelete(Texture2D* p_tex);

		// Intialize PxMaterial
		static void InitPhysXMaterialAsset(PhysXMaterialAsset& asset);

		static void LoadMeshAssetIntoGL(MeshAsset* asset);
		static Texture2D* CreateMeshAssetTexture(const std::string& dir, const aiTextureType& type, const aiMaterial* p_material);
		void IStallUntilMeshesLoaded();

		static void DispatchAssetEvent(Events::AssetEventType type, uint8_t* data_payload);

		void IClearAll();

		void InitBaseAssets();
		void InitBaseCube();
		void InitBaseTexture();
		void InitBase3DQuad();


		std::unique_ptr<ScriptAsset> mp_base_script = nullptr;
		std::unique_ptr<MeshAsset> mp_base_cube = nullptr;
		std::unique_ptr<MeshAsset> mp_base_sphere = nullptr;
		std::unique_ptr<MeshAsset> mp_base_quad = nullptr;
		std::unique_ptr<Texture2D> mp_base_tex = nullptr;
		std::unique_ptr<PhysXMaterialAsset> mp_base_physx_material = nullptr;
		std::unique_ptr<Texture2D> mp_base_brdf_lut = nullptr;

		// If a material fails to load etc, use this one instead
		std::unique_ptr<Material> mp_base_material = nullptr;

		// Default-initialize audio components with this sound asset
		std::unique_ptr<SoundAsset> mp_base_sound = nullptr;

		std::unordered_map<uint64_t, Asset*> m_assets;

		// Update listener checks if futures in m_mesh_loading_queue are ready and handles them if they are
		Events::EventListener<Events::EngineCoreEvent> m_update_listener;
		std::vector<std::future<MeshAsset*>> m_mesh_loading_queue;
		std::vector<std::future<void>> m_texture_futures;

		// Used for texture loading
		GLFWwindow* mp_loading_context = nullptr;
	};
}