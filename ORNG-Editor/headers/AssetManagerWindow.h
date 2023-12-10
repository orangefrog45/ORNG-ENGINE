#include "rendering/Textures.h"
#include "scene/Scene.h"
#include "events/EventManager.h"

namespace ORNG {
	class MeshAsset;
	class Material;

	struct ConfirmationWindowData {
		ConfirmationWindowData(const std::string t_str, std::function<void()> t_callback) : str(t_str), callback(t_callback) {};
		std::string str;
		std::function<void()> callback;
	};

	struct ErrorMessage {
		ErrorMessage(const char* err, std::vector<std::string>& logs) : error(err), prev_logs(std::move(logs)) {};
		std::string error;
		std::vector<std::string> prev_logs;
	};

	class AssetManagerWindow {
	public:
		AssetManagerWindow(std::string* p_active_project_dir, std::unique_ptr<Scene>& scene_context) : mp_active_project_dir(p_active_project_dir), mp_scene_context(&scene_context) {};
		// Renders previews, does not render the UI
		void OnMainRender();
		void OnRenderUI();
		void Init();

		void OnShutdown() {
			mp_preview_scene = nullptr;
		}

		void SelectMaterial(Material* p_material) {
			mp_selected_material = p_material;
		}
		void SelectTexture(Texture2D* p_texture) {
			mp_selected_texture = p_texture;
		}

		// Returns 0 if preview not found for material
		uint32_t GetMaterialPreviewTex(const Material* p_material) {
			return m_material_preview_textures.contains(p_material) ? m_material_preview_textures[p_material]->GetTextureHandle() : 0;
		}

		uint32_t GetMeshPreviewTex(const MeshAsset* p_mesh) {
			return m_mesh_preview_textures.contains(p_mesh) ? m_mesh_preview_textures[p_mesh]->GetTextureHandle() : 0;
		}
		inline static const int window_height = 300;
	private:
		void RenderMainAssetWindow();

		void InitPreviewScene();

		void CreateAndSerializePrefab(uint64_t entt_id, const std::string& fp);
		void RenderConfirmationWindow(ConfirmationWindowData& data, int index);
		void PushConfirmationWindow(const std::string& str, std::function<void()> func) {
			m_confirmation_window_stack.emplace_back(str, func);
		}
		void CreateMaterialPreview(const Material* p_material);
		void CreateMeshPreview(MeshAsset* p_asset);
		void OnProjectEvent(const Events::AssetEvent& t_event);
		bool RenderMaterialEditorSection();
		bool RenderMaterialTexture(const char* name, Texture2D*& p_tex);
		void RenderTextureEditorSection();

		void ReloadScript(const std::string& relative_path);
		void RenderScriptAsset(const std::filesystem::directory_entry& entry);
		void RenderScriptTab();

		void RenderMeshAssetTab();
		void RenderMeshAsset(MeshAsset* p_mesh_asset);

		void RenderPhysxMaterialTab();

		void RenderTextureTab();
		void RenderTexture(Texture2D* p_tex);

		void RenderMaterialTab();
		void RenderMaterial(Material* p_material);

		void RenderPhysXMaterialEditor();

		void RenderAudioTab();

		void RenderPrefabTab();

		void OnRequestDeleteAsset(Asset* p_asset) {
			PushConfirmationWindow("Delete asset?", [=] {
				// Cleanup binary file
				FileDelete(p_asset->filepath);

				m_asset_deletion_queue.push_back(p_asset->uuid());
				});
		};

		ImVec2 image_button_size{ 125, 125};
		unsigned column_count = 1;

		Texture2DSpec m_current_2d_tex_spec;
		Texture2D* mp_selected_texture = nullptr;
		Material* mp_selected_material = nullptr;
		PhysXMaterialAsset* mp_selected_physx_material = nullptr;

		std::unique_ptr<Scene>* mp_scene_context = nullptr;

		// UUID's of assets flagged for deletion
		std::vector<uint64_t> m_asset_deletion_queue;

		Events::EventListener<Events::AssetEvent> m_asset_listener;

		std::vector<ConfirmationWindowData> m_confirmation_window_stack;
		std::vector<ErrorMessage> m_error_messages;

		std::unique_ptr<Scene> mp_preview_scene = nullptr;

		// Should always be equal to working directory but just to be safe track explicitly from EditorLayer
		std::string* mp_active_project_dir = nullptr;

		std::unordered_map<const MeshAsset*, std::shared_ptr<Texture2D>> m_mesh_preview_textures;
		std::unordered_map<const Material*, std::shared_ptr<Texture2D>> m_material_preview_textures;
		// Have to store these in a vector and process at a specific stage otherwise ImGui breaks, any material in this vector will have a preview rendered for use in its imgui imagebutton
		std::vector<Material*> m_materials_to_gen_previews;
		std::vector<MeshAsset*> m_meshes_to_gen_previews;
		Texture2DSpec m_asset_preview_spec;
	};
}