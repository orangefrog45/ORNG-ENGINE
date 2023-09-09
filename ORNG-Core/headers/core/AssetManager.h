#include "rendering/Material.h"
#include "events/Events.h"
#include "scripting/ScriptingEngine.h"
#include "rendering/Textures.h"

#define ORNG_REPLACEMENT_MATERIAL_ID 0

class GLFWwindow;
enum aiTextureType;
class aiMaterial;

namespace FMOD {
	class Sound;
}

namespace ORNG {
	class Texture2D;
	class MeshAsset;
	class Texture2DSpec;

	struct SoundAsset {
		// Sound will be provided by AssetManager
		SoundAsset(FMOD::Sound* t_p_sound, const std::string& t_filepath) : p_sound(t_p_sound), filepath(t_filepath) {};
		~SoundAsset();
		std::string filepath;
		FMOD::Sound* p_sound = nullptr;
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
		inline static Material* CreateMaterial(uint64_t uuid = 0) { return Get().ICreateMaterial(uuid); }
		inline static Material* GetMaterial(uint64_t uuid) { return Get().IGetMaterial(uuid); }
		static Material* GetEmptyMaterial() { return &Get().m_replacement_material; }
		inline static void DeleteMaterial(uint64_t uuid) { Get().IDeleteMaterial(uuid); }
		inline static void DeleteMaterial(Material* p_material) { Get().IDeleteMaterial(p_material->uuid()); }

		inline static Texture2D* CreateTexture2D(const Texture2DSpec& spec, uint64_t uuid = 0) { return Get().ICreateTexture2D(spec, uuid); };
		inline static Texture2D* GetTexture(uint64_t uuid) { return Get().IGetTexture(uuid); }
		inline static void DeleteTexture(uint64_t uuid) { Get().IDeleteTexture(uuid); }
		static void DeleteTexture(Texture2D* p_tex);

		inline static MeshAsset* CreateMeshAsset(const std::string& filename, uint64_t uuid = 0) { return Get().ICreateMeshAsset(filename, uuid); }
		inline static MeshAsset* GetMeshAsset(uint64_t uuid) { return Get().IGetMeshAsset(uuid); }
		static void LoadMeshAsset(MeshAsset* p_asset);
		inline static void DeleteMeshAsset(uint64_t uuid) { Get().IDeleteMeshAsset(uuid); }
		static void DeleteMeshAsset(MeshAsset* p_mesh);

		static SoundAsset* AddSoundAsset(const std::string& filepath);
		static SoundAsset* GetSoundAsset(const std::string& filepath);
		static void DeleteSoundAsset(SoundAsset* p_asset);

		static ScriptSymbols* AddScriptAsset(const std::string& filepath);
		// Returns either the script asset referenced with the filepath or nullptr if none found
		static ScriptSymbols* GetScriptAsset(const std::string& filepath);
		static bool DeleteScriptAsset(const std::string& filepath);

		// Deletes all assets
		inline static void ClearAll() { Get().IClearAll(); };

		// Stalls program and waits for meshes to load - this will cause the program to freeze
		inline static void StallUntilMeshesLoaded() { Get().IStallUntilMeshesLoaded(); }

		static void SerializeMeshAssetBinary(const std::string& filepath, MeshAsset& data);
		static void DeserializeMeshAssetBinary(const std::string& filepath, MeshAsset& data);


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
		static Texture2D* CreateMeshAssetTexture(const std::string& dir, const aiTextureType& type, const aiMaterial* p_material);
		void IStallUntilMeshesLoaded();


		static void DispatchAssetEvent(Events::AssetEventType type, uint8_t* data_payload);

		void IClearAll();

		Material m_replacement_material{ (uint64_t)ORNG_REPLACEMENT_MATERIAL_ID };

		std::vector<Material*> m_materials;
		std::vector<MeshAsset*> m_meshes;
		std::vector<Texture2D*> m_2d_textures;
		std::vector<SoundAsset*> m_sound_assets;


		// Update listener checks if futures in m_mesh_loading_queue are ready and handles them if they are
		Events::EventListener<Events::EngineCoreEvent> m_update_listener;
		std::vector<std::future<MeshAsset*>> m_mesh_loading_queue;
		std::vector<std::future<void>> m_texture_futures;

		// Key=filepath
		std::unordered_map<std::string, ScriptSymbols*> m_scripts;
		// Used for texture loading
		GLFWwindow* mp_loading_context = nullptr;
	};


}