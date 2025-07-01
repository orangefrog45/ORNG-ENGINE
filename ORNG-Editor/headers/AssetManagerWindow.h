#pragma once
#include <assets/PhysXMaterialAsset.h>
#include <assets/SoundAsset.h>

#include "rendering/Textures.h"
#include "scene/Scene.h"
#include "events/Events.h"

namespace ORNG {
	class MeshAsset;
	class Material;
	struct ScriptAsset;
	struct Prefab;

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

	struct ImGuiPopupSpec {
		std::vector<std::pair<const char*, std::function<void()>>> options;
	};

	struct AssetDisplaySpec {
		Texture2D* p_tex = nullptr;
		std::function<void()> on_delete = nullptr;
		std::function<void()> on_click = nullptr;
		std::function<void()> on_drag = nullptr;
		std::function<void()> on_drop = nullptr;

		std::function<void()> on_name_clicked = nullptr;

		ImGuiPopupSpec popup_spec;
		std::string delete_confirmation_str;
		
		std::string override_name;
	};

	struct AssetAddDisplaySpec {
		std::function<void()> on_render = nullptr;
	};

	class AssetManagerWindow {
		friend class EditorLayer;
	public:
		AssetManagerWindow(std::string* p_active_project_dir, Scene* p_scene, class EditorLayer* p_editor) :
			mp_active_project_dir(p_active_project_dir), mp_scene_context(p_scene), mp_editor(p_editor) {};
		// Renders previews, does not render the UI
		void OnMainRender();
		void OnRenderUI();
		void Init();

		void SetScene(Scene* p_scene) {
			mp_scene_context = p_scene;
		}

		void OnShutdown() {
			mp_preview_scene->UnloadScene();
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

		Scene* p_extern_scene = nullptr;

	private:
		void RenderMainAssetWindow();

		void InitPreviewScene();

		// Prefabs can contain one entity with all its children, but not multiple unrelated entities
		bool CreateAndSerializePrefab(SceneEntity& entity, const std::string& fp, uint64_t uuid = 0);

		void RenderConfirmationWindow(ConfirmationWindowData& data, int index);
		void PushConfirmationWindow(const std::string& str, const std::function<void()>& func) {
			m_confirmation_window_stack.emplace_back(str, func);
		}
		void CreateMaterialPreview(const Material* p_material);
		void CreateMeshPreview(MeshAsset* p_asset);
		void OnProjectEvent(const Events::AssetEvent& t_event);
		bool RenderMaterialEditorSection();
		bool RenderMaterialTexture(const char* name, Texture2D*& p_tex);
		void RenderTextureEditorSection();

		bool RenderDirectory(const std::filesystem::path& path, std::string& active_path);
		void RenderAsset(Asset* p_asset);
		void RenderBaseAsset(Asset* p_asset, const AssetDisplaySpec& display_spec);

		void RenderAddAssetPopup(bool open_condition);
		bool RenderAddMeshAssetWindow();
		bool RenderAddTexture2DAssetWindow();
		bool RenderAddMaterialAssetWindow();
		bool RenderAddPrefabAssetWindow();
		bool RenderAddScriptAssetWindow();
		bool RenderAddPhysxMaterialAssetWindow();
		bool RenderAddSoundAssetWindow();

		bool RenderBaseAddAssetWindow(const AssetAddDisplaySpec& display_spec, std::string& name, std::string& filepath);
		bool CanCreateAsset(const std::string& asset_filepath);

		void UnloadScript(ScriptAsset& script);
		void LoadScript(ScriptAsset& asset, const std::string& relative_path);
		void UnloadScriptFromComponents(const std::string& relative_path);

		void RenderScriptAsset(ScriptAsset* p_asset);
		void RenderMeshAsset(MeshAsset* p_mesh_asset);
		void RenderPhysXMaterial(class PhysXMaterialAsset* p_material);
		void RenderTexture(Texture2D* p_tex);
		void RenderMaterial(Material* p_material);
		void RenderPrefab(Prefab* p_prefab);
		void RenderAudioAsset(class SoundAsset* p_asset);

		void RenderPhysXMaterialEditor();

		void ProcessAssetDeletionQueue();

		void OnRequestDeleteAsset(Asset* p_asset, const std::string& confirmation_text, const std::function<void()>& callback = nullptr);

		ImVec2 image_button_size{ 125, 125};
		unsigned column_count = 1;

		std::string m_current_content_dir = "res";

		Texture2DSpec m_current_2d_tex_spec;
		Texture2D* mp_selected_texture = nullptr;
		Material* mp_selected_material = nullptr;
		PhysXMaterialAsset* mp_selected_physx_material = nullptr;

		Scene* mp_scene_context = nullptr;
		EditorLayer* mp_editor = nullptr;

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