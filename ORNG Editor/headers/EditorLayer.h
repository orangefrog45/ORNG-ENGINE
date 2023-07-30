#pragma once
#include "EngineAPI.h"
#include "../extern/imgui/imgui.h"
#include "EditorCamera.h"

namespace physx {
	class PxMaterial;
}

namespace ORNG {
	class Shader;
	class Framebuffer;
	class Renderer;

	class EditorLayer : public Layer {
	public:
		void Init();

	private:
		void OnInit() override { Init(); };
		void Update() override;
		void OnRender() override { RenderDisplayWindow(); RenderUI(); };

		void InitImGui();
		void RenderDisplayWindow();
		void RenderUI();
		void RenderEditorWindow();

		void RenderGrid();
		void DoPickingPass();
		/* Highlight the selected entities in the editor */
		void DoSelectedEntityHighlightPass();

		// Renders a popup with shortcuts to create a new entity with a component, e.g mesh
		void RenderCreationWidget(SceneEntity* p_entity, bool trigger);
		void DisplayEntityEditor();
		void RenderMeshComponentEditor(MeshComponent* comp);
		void RenderPointlightEditor(PointLightComponent* light);
		void RenderSpotlightEditor(SpotLightComponent* light);
		void RenderCameraEditor(CameraComponent* p_cam);
		void RenderTransformComponentEditor(std::vector<TransformComponent*>& transforms);
		void RenderPhysicsComponentEditor(PhysicsComponent* p_comp);
		void RenderPhysicsMaterial(physx::PxMaterial* p_material);

		// Renders material as a drag-drop target, returns pointer to the new material if a material was drag-dropped on it, else nullptr
		Material* RenderMaterialComponent(const Material* p_material);

		SceneEntity* DuplicateEntity(SceneEntity* p_original);

		void RenderSceneGraph();
		void RenderProfilingTimers();
		void RenderSkyboxEditor();
		void RenderEntityNode(SceneEntity* p_entity);
		void RenderDirectionalLightEditor();
		void RenderGlobalFogEditor();
		void RenderBloomEditor();
		void RenderTerrainEditor();

#define INVALID_ENTITY_ID 0

		inline void SelectEntity(uint64_t id) {
			if (!VectorContains(m_selected_entity_ids, id) && id != INVALID_ENTITY_ID)
				m_selected_entity_ids.push_back(id);
			else
				return;

			if (!m_selected_entity_ids.empty())
				// Makes the selected entity the first ID, which some UI components will operate on more, e.g gizmos will render on this entity now over other selected ones
				std::iter_swap(std::ranges::find(m_selected_entity_ids, id), m_selected_entity_ids.begin());
		}


		inline void DeselectEntity(uint64_t id) {
			auto it = std::ranges::find(m_selected_entity_ids, id);
			if (it != m_selected_entity_ids.end())
				m_selected_entity_ids.erase(it);
		}


		void ShowAssetManager();
		void RenderTextureEditorSection();
		void RenderMaterialEditorSection();



		/// Basic file explorer, success callback called when a file is clicked that has a valid extension, fail callback called when file has invalid extension.
		/// Always provide extensions fully capitalized.
		void ShowFileExplorer(std::string& path_ref, wchar_t extension_filter[], std::function<void()> valid_file_callback);

		static bool ShowVec3Editor(const char* name, glm::vec3& vec, float min = std::numeric_limits<float>::lowest(), float max = std::numeric_limits<float>::max());
		static bool ShowVec2Editor(const char* name, glm::vec2& vec, float min = std::numeric_limits<float>::lowest(), float max = std::numeric_limits<float>::max());
		static bool ShowColorVec3Editor(const char* name, glm::vec3& vec);
		void RenderMaterialTexture(const char* name, Texture2D*& p_tex, bool deletable);
		static bool H1TreeNode(const char* name);
		static bool H2TreeNode(const char* name);
		static bool ClampedFloatInput(const char* name, float* p_val, float min = std::numeric_limits<float>::lowest(), float max = std::numeric_limits<float>::max());
		// Creates an empty imgui tree node
		static bool EmptyTreeNode(const char* name);


		Shader* mp_quad_shader = nullptr;
		Shader* mp_picking_shader = nullptr;
		Shader* mp_grid_shader = nullptr;
		Shader* mp_highlight_shader = nullptr;

		Framebuffer* mp_editor_pass_fb = nullptr; // Framebuffer that any editor stuff will be rendered into e.g grid
		Framebuffer* mp_picking_fb = nullptr;

		std::unique_ptr<GridMesh> m_grid_mesh = nullptr;
		std::unique_ptr<Scene> m_active_scene = nullptr;

		std::unique_ptr<SceneEntity> mp_editor_camera{nullptr };

		uint64_t m_mouse_selected_entity_id = 0;

		std::vector<uint64_t> m_selected_entity_ids;
		bool m_selected_entities_are_dragged = false;

		bool m_display_directional_light_editor = false;
		bool m_display_skybox_editor = false;
		bool m_display_global_fog_editor = false;
		bool m_display_terrain_editor = false;
		bool m_display_bloom_editor = false;

		struct DisplayWindowSettings {
			bool depth_map_view = false;
		};




		// UI VARS
		Texture2D* mp_selected_texture = nullptr;
		Texture2D* mp_dragged_texture = nullptr;
		Texture2DSpec m_current_2d_tex_spec;
		Material* mp_selected_material = nullptr;
		Material* mp_dragged_material = nullptr;


		// STYLING
		const float opacity = 1.f;
		const ImVec4 orange_color = ImVec4(0.9f, 0.2f, 0.05f, opacity);
		const ImVec4 orange_color_bright = ImVec4(0.9f, 0.2f, 0.0f, opacity);
		const ImVec4 orange_color_brightest = ImVec4(0.9f, 0.2f, 0.0f, opacity);
		const ImVec4 dark_grey_color = ImVec4(0.1f, 0.1f, 0.1f, opacity);
		const ImVec4 lighter_grey_color = ImVec4(0.2f, 0.2f, 0.2f, opacity);
		const ImVec4 lightest_grey_color = ImVec4(0.3f, 0.3f, 0.3f, opacity);
		glm::vec2 file_explorer_window_size = { 750, 750 };
	};
}