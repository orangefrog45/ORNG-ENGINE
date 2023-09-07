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

	class AssetManagerWindow {
	public:
		AssetManagerWindow(std::string* p_active_project_dir) : mp_active_project_dir(p_active_project_dir) {};
		// Renders previews, does not render the UI
		void OnMainRender();
		void OnRenderUI();
		void Init();

		// Returns 0 if preview not found for material
		uint32_t GetMaterialPreviewTex(const Material* p_material) {
			return m_material_preview_textures.contains(p_material) ? m_material_preview_textures[p_material]->GetTextureHandle() : 0;
		}

		uint32_t GetMeshPreviewTex(const MeshAsset* p_mesh) {
			return m_mesh_preview_textures.contains(p_mesh) ? m_mesh_preview_textures[p_mesh]->GetTextureHandle() : 0;
		}

	private:
		void RenderMainAssetWindow();
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
		Texture2DSpec m_current_2d_tex_spec;
		Texture2D* mp_selected_texture = nullptr;
		Material* mp_selected_material = nullptr;

		Events::EventListener<Events::AssetEvent> m_asset_listener;

		std::vector<ConfirmationWindowData> m_confirmation_window_stack;

		std::unique_ptr<Scene> mp_preview_scene = nullptr;

		// Should always be equal to working directory but just to be safe track explicitly from EditorLayer
		std::string* mp_active_project_dir = nullptr;

		std::unordered_map<const MeshAsset*, std::shared_ptr<Texture2D>> m_mesh_preview_textures;
		std::unordered_map<const Material*, std::shared_ptr<Texture2D>> m_material_preview_textures;
		// Have to store these in a vector and process at a specific stage otherwise ImGui breaks, any material in this vector will have a preview rendered for use in its imgui imagebutton
		std::vector<Material*> m_materials_to_gen_previews;
		Texture2DSpec m_asset_preview_spec;

	};

}