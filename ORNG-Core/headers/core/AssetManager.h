#include "rendering/Textures.h"
#include "rendering/MeshAsset.h"
#include "events/Events.h"

#define ORNG_REPLACEMENT_MATERIAL_ID 0

class GLFWwindow;
namespace ORNG {

	class AssetManager {
	public:
		friend class EditorLayer;
		friend class SceneSerializer;
		static AssetManager& Get() {
			static AssetManager s_instance;
			return s_instance;
		}

		static void Init() { Get().I_Init(); }
		inline static Material* CreateMaterial(uint64_t uuid = 0) { return Get().ICreateMaterial(uuid); }
		inline static Material* GetMaterial(uint64_t uuid) { return Get().IGetMaterial(uuid); }
		static Material* GetEmptyMaterial() { return &Get().m_replacement_material; }
		inline static void DeleteMaterial(uint64_t uuid) { Get().IDeleteMaterial(uuid); }
		inline static void DeleteMaterial(Material* p_material) { Get().IDeleteMaterial(p_material->uuid()); }

		inline static Texture2D* CreateTexture2D(const Texture2DSpec& spec, uint64_t uuid = 0) { return Get().ICreateTexture2D(spec, uuid); };
		inline static Texture2D* GetTexture(uint64_t uuid) { return Get().IGetTexture(uuid); }
		inline static void DeleteTexture(uint64_t uuid) { Get().IDeleteTexture(uuid); }
		inline static void DeleteTexture(Texture2D* p_tex) { Get().IDeleteTexture(p_tex->uuid()); }

		inline static MeshAsset* CreateMeshAsset(const std::string& filename, uint64_t uuid = 0) { return Get().ICreateMeshAsset(filename, uuid); }
		inline static MeshAsset* GetMeshAsset(uint64_t uuid) { return Get().IGetMeshAsset(uuid); }
		static void LoadMeshAsset(MeshAsset* p_asset);
		inline static void DeleteMeshAsset(uint64_t uuid) { Get().IDeleteMeshAsset(uuid); }
		inline static void DeleteMeshAsset(MeshAsset* p_mesh) { Get().IDeleteMeshAsset(p_mesh->uuid()); }

		// Deletes all assets
		inline static void ClearAll() { Get().IClearAll(); };

		// Stalls program and waits for meshes to load - this will cause the program to freeze
		inline static void StallUntilMeshesLoaded() { Get().IStallUntilMeshesLoaded(); }

	private:
		void I_Init();
		Material* ICreateMaterial(uint64_t uuid = 0);
		Material* IGetMaterial(uint64_t uuid);
		static const Material* IGetEmptyMaterial() { return &Get().m_replacement_material; }
		void IDeleteMaterial(uint64_t uuid);

		Texture2D* ICreateTexture2D(const Texture2DSpec& spec, uint64_t uuid = 0);
		Texture2D* IGetTexture(uint64_t uuid);
		void IDeleteTexture(uint64_t uuid);

		MeshAsset* ICreateMeshAsset(const std::string& filename, uint64_t uuid = 0);
		MeshAsset* IGetMeshAsset(uint64_t uuid);
		void IDeleteMeshAsset(uint64_t uuid);
		static void LoadMeshAssetIntoGL(MeshAsset* asset, std::vector<Material*>& materials);
		static Texture2D* CreateMeshAssetTexture(const std::string& dir, aiTextureType type, const aiMaterial* p_material);
		// Loads a mesh with other materials than contained in the files on disc, usually for deserialization after they've been modified in the editor.
		static void LoadMeshAssetPreExistingMaterials(MeshAsset* asset, std::vector<Material*>& materials);
		void IStallUntilMeshesLoaded();

		static void DispatchAssetEvent(Events::ProjectEventType type, uint8_t* data_payload);

		void IClearAll();

		Material m_replacement_material{ (uint64_t)ORNG_REPLACEMENT_MATERIAL_ID };

		std::vector<Material*> m_materials;
		std::vector<MeshAsset*> m_meshes;
		std::vector<Texture2D*> m_2d_textures;

		struct MeshAssetPackage {
			MeshAssetPackage(MeshAsset* t_asset, std::vector<Material*> t_materials) : p_asset(t_asset), materials(t_materials) {}; // Copying materials here instead of ref due to async code
			MeshAsset* p_asset = nullptr;
			// Materials will be used if "LoadMeshAssetPreExistingMaterials" called
			std::vector<Material*> materials;
		};

		// Update listener checks if futures in m_mesh_loading_queue are ready and handles them if they are
		Events::EventListener<Events::EngineCoreEvent> m_update_listener;
		std::vector<std::future<MeshAssetPackage>> m_mesh_loading_queue;
		std::vector<std::future<void>> m_texture_futures;
		// Used for texture loading
		GLFWwindow* mp_loading_context = nullptr;
	};


}