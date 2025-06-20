#include "rendering/Material.h"
#include "events/Events.h"
#include "scripting/ScriptingEngine.h"
#include "assets/SoundAsset.h"
#include "AssetSerializer.h"

#define ORNG_BASE_MATERIAL_ID 0
#define ORNG_BASE_SOUND_ID 1
#define ORNG_BASE_TEX_ID 2
#define ORNG_BASE_CUBE_ID 3
#define ORNG_BASE_SCRIPT_ID	4
#define ORNG_BASE_SPHERE_ID 5
#define ORNG_BASE_PHYSX_MATERIAL_ID 6
#define ORNG_BASE_QUAD_ID 7
#define ORNG_BASE_BRDF_LUT_ID 8
#define ORNG_NUM_BASE_ASSETS 9

struct GLFWwindow;
enum aiTextureType;
struct aiMaterial;

namespace ORNG {
	class Texture2D;
	class MeshAsset;
	struct Texture2DSpec;

	class AssetManager {
	public:
		friend class AssetManagerWindow;
		friend class EditorLayer;
		friend class SceneSerializer;
		friend class AssetSerializer;

		static AssetManager& Get() {
			DEBUG_ASSERT(mp_instance);
			return *mp_instance;
		}

		static AssetSerializer& GetSerializer() {
			return Get().serializer;
		}

		static void Init(AssetManager* p_instance = nullptr) { 
			if (p_instance) {
				ASSERT(!mp_instance);
				mp_instance = p_instance;
			}
			else {
				mp_instance = new AssetManager();
				Get().I_Init(); 
			}

		}

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
		static void Shutdown() {
			Get().IOnShutdown();
		};

		static void HandleAssetDeletion(Asset* p_asset);
		static void HandleAssetAddition(Asset* p_asset);

		// Deletes all assets
		inline static void ClearAll() { Get().IClearAll(); };

		AssetSerializer serializer{ *this };
	private:
		void I_Init();
		void IOnShutdown();

		// Loads all base assets (assets the engine runtime requires) that require an external file, e.g the sphere mesh needs to be loaded from a binary file. 
		// These files are always present in the "res/core-res" folder of a project
		void LoadExternalBaseAssets(const std::string& project_dir);

		static void OnTextureDelete(Texture2D* p_tex);

		static void DispatchAssetEvent(Events::AssetEventType type, uint8_t* data_payload);

		void IClearAll();

		void InitBaseAssets();
		void InitBaseTexture();
		void InitBase3DQuad();
		
		inline static AssetManager* mp_instance = nullptr;

		std::unique_ptr<ScriptAsset> mp_base_script = nullptr;
		std::unique_ptr<MeshAsset> mp_base_sphere = nullptr;
		std::unique_ptr<MeshAsset> mp_base_cube = nullptr;
		std::unique_ptr<MeshAsset> mp_base_quad = nullptr;
		std::unique_ptr<Texture2D> mp_base_tex = nullptr;
		std::unique_ptr<Texture2D> mp_base_brdf_lut = nullptr;

		// If a material fails to load etc, use this one instead
		std::unique_ptr<Material> mp_base_material = nullptr;

		// Default-initialize audio components with this sound asset
		std::unique_ptr<SoundAsset> mp_base_sound = nullptr;

		std::unordered_map<uint64_t, Asset*> m_assets;

		// Update listener checks if futures in m_mesh_loading_queue are ready and handles them if they are
		Events::EventListener<Events::EngineCoreEvent> m_update_listener;
	};
}