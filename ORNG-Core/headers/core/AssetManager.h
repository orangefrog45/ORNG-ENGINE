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
		inline static Material* CreateMaterial(uint64_t uuid = 0) { return Get().ICreateMaterial(); }
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
		static void LoadMeshAssetIntoGL(MeshAsset* asset);
		static Texture2D* LoadMeshAssetTexture(const std::string& dir, aiTextureType type, const aiMaterial* p_material);
		static void LoadMeshAssetPreExistingMaterials(MeshAsset* asset, std::vector<Material*>& materials);

		Material m_replacement_material{ (uint64_t)ORNG_REPLACEMENT_MATERIAL_ID };

		std::vector<Material*> m_materials;
		std::vector<MeshAsset*> m_meshes;
		std::vector<Texture2D*> m_2d_textures;

		// Update listener checks if futures in m_mesh_loading_queue are ready and handles them if they are
		Events::EventListener<Events::EngineCoreEvent> m_update_listener;
		std::vector<std::future<MeshAsset*>> m_mesh_loading_queue;
		// Used for texture loading
		GLFWwindow* mp_loading_context = nullptr;
	};


}