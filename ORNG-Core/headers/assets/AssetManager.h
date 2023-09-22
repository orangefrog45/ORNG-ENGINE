#include "rendering/Material.h"
#include "events/Events.h"
#include "scripting/ScriptingEngine.h"
#include "rendering/Textures.h"
#include "rendering/MeshAsset.h"
#include <bitsery/bitsery.h>
#include <bitsery/traits/vector.h>
#include <bitsery/adapter/stream.h>
#include "bitsery/traits/string.h"

#define ORNG_REPLACEMENT_MATERIAL_ID 0

class GLFWwindow;
enum aiTextureType;
class aiMaterial;

namespace FMOD {
	class Sound;
}


namespace bitsery {
	using namespace ORNG;
	template <typename S>
	void serialize(S& s, glm::vec3& o) {
		s.value4b(o.x);
		s.value4b(o.y);
		s.value4b(o.z);
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
	class Texture2DSpec;

	struct Prefab : public Asset {

		Prefab(const std::string& filepath) : Asset(filepath) {};
		// Yaml string that can be deserialized into entity
		std::string serialized_content;

		template<typename S>
		void serialize(S& s) {
			s.text1b(serialized_content, ORNG_MAX_FILEPATH_SIZE);
			s.object(uuid);
			s.text1b(filepath, ORNG_MAX_FILEPATH_SIZE);
		}
	};

	struct SoundAsset : public Asset {
		SoundAsset(const std::string& t_filepath) : Asset(t_filepath) {};
		~SoundAsset();

		FMOD::Sound* p_sound = nullptr;

		template<typename S>
		void serialize(S& s) {
			s.text1b(filepath, ORNG_MAX_FILEPATH_SIZE);
			s.object(uuid);
		}

		void CreateSound();
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
				if (T* p_typed_asset = dynamic_cast<T*>(p_asset))
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
			bool found = false;
			for (auto [key, val] : Get().m_assets) {
				if (val == dynamic_cast<T*>(p_asset)) {
					HandleAssetDeletion(val);
					delete p_asset;
					Get().m_assets.erase(key);
					found = true;
					break;
				}
			}

			return found;
		}

		static Material* GetEmptyMaterial() { return &Get().m_replacement_material; }

		static void HandleAssetDeletion(Asset* p_asset);
		static void HandleAssetAddition(Asset* p_asset);
		static void LoadMeshAsset(MeshAsset* p_asset);
		static void LoadTexture2D(Texture2D* p_tex);
		static void LoadAssetsFromProjectPath(const std::string& project_dir);
		static void SerializeAssets();

		// Deletes all assets
		inline static void ClearAll() { Get().IClearAll(); };

		// Stalls program and waits for meshes to load - this will cause the program to freeze
		inline static void StallUntilMeshesLoaded() { Get().IStallUntilMeshesLoaded(); }

		template <std::derived_from<Asset> T>
		static void SerializeAssetBinary(const std::string& filepath, T& asset) {
			std::ofstream s{ filepath, s.binary | s.trunc | s.out };
			if (!s.is_open()) {
				ORNG_CORE_ERROR("Vertex serialization error: Cannot open {0} for writing", filepath);
				return;
			}
			bitsery::Serializer<bitsery::OutputBufferedStreamAdapter> ser{ s };

			ser.object(asset);
			// flush to writer
			ser.adapter().flush();
			s.close();
		}

		template <std::derived_from<Asset> T>
		static void DeserializeAssetBinary(const std::string& filepath, T& data) {
			std::ifstream s{ filepath, std::ios::binary };
			if (!s.is_open()) {
				ORNG_CORE_ERROR("Deserialization error: Cannot open {0} for reading", filepath);
				return;
			}

			// Use buffered stream adapter
			bitsery::Deserializer<bitsery::InputStreamAdapter> des{ s };

			if constexpr (std::is_same_v<T, MeshAsset>) {
				des.object(data.m_vao);
				des.object(data.m_aabb);
				uint32_t size;
				des.value4b(size);
				data.m_submeshes.resize(size);
				for (int i = 0; i < size; i++) {
					des.object(data.m_submeshes[i]);
				}
				des.value1b(data.num_materials);
				des.object(data.uuid);
				des.container1b(data.filepath, 10000);
			}
			else if constexpr (std::is_same_v<T, Material>) {
				des.object(data.base_color);
				des.value4b(data.roughness);
				des.value4b(data.metallic);
				des.value4b(data.ao);
				des.value1b(data.emissive);
				des.value4b(data.emissive_strength);
				uint64_t texid;
				des.value8b(texid);
				data.base_color_texture = GetAsset<Texture2D>(texid);
				des.value8b(texid);
				data.normal_map_texture = GetAsset<Texture2D>(texid);
				des.value8b(texid);
				data.metallic_texture = GetAsset<Texture2D>(texid);
				des.value8b(texid);
				data.roughness_texture = GetAsset<Texture2D>(texid);
				des.value8b(texid);
				data.ao_texture = GetAsset<Texture2D>(texid);
				des.value8b(texid);
				data.displacement_texture = GetAsset<Texture2D>(texid);
				des.value8b(texid);
				data.emissive_texture = GetAsset<Texture2D>(texid);

				des.value4b(data.parallax_layers);
				des.value4b(data.parallax_height_scale);
				des.object(data.tile_scale);
				des.text1b(data.name, ORNG_MAX_NAME_SIZE);
				des.object(data.uuid);
				des.text1b(data.filepath, ORNG_MAX_NAME_SIZE);
			}
			else {
				des.object(data);
			}

			if constexpr (std::is_same_v<T, Texture2D>) {
				// Needed to get the state to update properly with opengl
				data.SetSpec(data.GetSpec());
			}
		}



	private:
		void I_Init();

		static void OnTextureDelete(Texture2D* p_tex);

		static const Material* IGetEmptyMaterial() { return &Get().m_replacement_material; }
		static void LoadMeshAssetIntoGL(MeshAsset* asset);
		static Texture2D* CreateMeshAssetTexture(const std::string& dir, const aiTextureType& type, const aiMaterial* p_material);
		void IStallUntilMeshesLoaded();

		static void DispatchAssetEvent(Events::AssetEventType type, uint8_t* data_payload);

		void IClearAll();

		Material m_replacement_material{ (uint64_t)ORNG_REPLACEMENT_MATERIAL_ID };

		std::unordered_map<uint64_t, Asset*> m_assets;

		// Update listener checks if futures in m_mesh_loading_queue are ready and handles them if they are
		Events::EventListener<Events::EngineCoreEvent> m_update_listener;
		std::vector<std::future<MeshAsset*>> m_mesh_loading_queue;
		std::vector<std::future<void>> m_texture_futures;

		// Used for texture loading
		GLFWwindow* mp_loading_context = nullptr;
	};


}