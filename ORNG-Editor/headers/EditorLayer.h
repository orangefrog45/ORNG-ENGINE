#pragma once
#include "EngineAPI.h"
#include "../extern/imgui/imgui.h"
#include "AssetManagerWindow.h"
#include "scene/GridMesh.h"
#include "Settings.h"
#include "EditorEventStack.h"

namespace physx {
	class PxMaterial;
}
struct DragData {
	glm::ivec2 start;
	glm::ivec2 end;
};




namespace ORNG {
	class Shader;
	class Framebuffer;
	class Renderer;

	class EditorLayer : public Layer {
	public:
		void Init();
		enum EntityNodeEvent {
			E_NONE = 0,
			E_DELETE = 1,
			E_DUPLICATE = 2
		};

	private:
		void OnInit() override { Init(); };
		void Update() override;
		void OnImGuiRender() override { RenderUI(); };
		void OnRender() override { RenderDisplayWindow(); };
		void OnShutdown() override;

		// Sets up directory structure for a new project
		bool GenerateProject(const std::string& project_name);
		// Sets working directory to folder_path so all resources will be found there.
		bool MakeProjectActive(const std::string& folder_path);
		void InitImGui();
		// Window containing the actual rendered scene
		void RenderDisplayWindow();
		void RenderToolbar();
		void RenderUI();
		void RenderSceneDisplayPanel();
		// Movement
		void UpdateEditorCam();

		void MultiSelectDisplay();

		void RenderGrid();
		void DoPickingPass();

		/* Highlight the selected entities in the editor */
		void DoSelectedEntityHighlightPass();
		void RenderPhysxDebug();

		void BeginPlayScene();
		void EndPlayScene();

		void RenderGeneralSettingsMenu();


		// Renders a popup with shortcuts to create a new entity with a component, e.g mesh
		void RenderCreationWidget(SceneEntity* p_entity, bool trigger);
		void DisplayEntityEditor();
		void RenderMeshComponentEditor(MeshComponent* comp);
		bool RenderMeshWithMaterials(const MeshAsset* p_asset, std::vector<const Material*>& materials, std::function<void(MeshAsset* p_new)> OnMeshDrop, std::function<void(unsigned index, Material* p_new)> OnMaterialDrop);
		void RenderPointlightEditor(PointLightComponent* light);
		void RenderSpotlightEditor(SpotLightComponent* light);
		void RenderCameraEditor(CameraComponent* p_cam);
		void RenderTransformComponentEditor(std::vector<TransformComponent*>& transforms);
		void RenderPhysicsComponentEditor(PhysicsComponent* p_comp);
		void RenderScriptComponentEditor(ScriptComponent* p_script);
		void RenderAudioComponentEditor(AudioComponent* p_audio);
		void RenderVehicleComponentEditor(VehicleComponent* p_comp);
		void RenderParticleEmitterComponentEditor(ParticleEmitterComponent* p_comp);

		void RenderProjectGenerator(int& selected_component_from_popup);

		// Ensures a project directory is in the correct state to be used in the editor, attempts repairs if not
		bool ValidateProjectDir(const std::string& dir_path);

		// Renders material as a drag-drop target, returns ptr of the new material if a material was drag-dropped on it, else nullptr
		Material* RenderMaterialComponent(const Material* p_material);

		void RenderSceneGraph();
		void RenderSkyboxEditor();

		EntityNodeEvent RenderEntityNode(SceneEntity* p_entity, unsigned int layer);
		void RenderDirectionalLightEditor();
		void RenderGlobalFogEditor();
		void RenderBloomEditor();
		void RenderTerrainEditor();

		void DeleteEntitiesTracked(std::vector<uint64_t> entities); // Copy vector as this will usually be m_selected_entity_id's and deselecting entities will modify it, throwing off the range-based for loop
		std::vector<SceneEntity*> DuplicateEntitiesTracked(std::vector<uint64_t> entities); // Copy vector for same reason as above

		// Duplicates entity and conditionally calls OnCreate on it depending on if simulate mode is active
		SceneEntity& CreateEntityTracked(const std::string& name);

		glm::vec2 ConvertFullscreenMouseToDisplayMouse(glm::vec2 mouse_coords);

#define INVALID_ENTITY_ID 0

		inline void SelectEntity(uint64_t id) {
			if (!VectorContains(m_selected_entity_ids, id) && id != INVALID_ENTITY_ID)
				m_selected_entity_ids.push_back(id);
			else if (!m_selected_entity_ids.empty())
				// Makes the selected entity the first ID, which some UI components will operate on more, e.g gizmos will render on this entity now over other selected ones
				std::iter_swap(std::ranges::find(m_selected_entity_ids, id), m_selected_entity_ids.begin());
		}


		inline void DeselectEntity(uint64_t id) {
			auto it = std::ranges::find(m_selected_entity_ids, id);
			if (it != m_selected_entity_ids.end())
				m_selected_entity_ids.erase(it);
		}

		DragData m_mouse_drag_data;

		GeneralSettings m_general_settings;
		EditorEventStack m_event_stack;

		AssetManagerWindow m_asset_manager_window{ &m_current_project_directory, m_active_scene };

		// Size, not position
		ImVec2 m_scene_display_rect{ 1, 1 };

		// Stores temporary serialized yaml data to load back in after exiting "play mode"
		std::string m_temp_scene_serialization;

		// If true editor will start simulating the scene as if it were running in a runtime layer
		bool m_simulate_mode_active = false;
		bool m_simulate_mode_paused = false;

		// Whenever simulation mode is active this will be true
		bool m_fullscreen_scene_display = false;
		// In simulation mode, determines if UI will be rendered on top. UI is always rendered in non-simulation mode, no matter this setting
		bool m_render_ui = false;

		// Texture spec for rendering the scene
		Texture2DSpec m_color_render_texture_spec;

		std::string m_executable_directory;
		// Working directory will always be this
		std::string m_current_project_directory;

		ImFont* mp_large_font = nullptr;

		Shader* mp_quad_shader = nullptr;
		Shader* mp_picking_shader = nullptr;
		Shader* mp_grid_shader = nullptr;
		Shader* mp_highlight_shader = nullptr;
		Shader* mp_quad_col_shader = nullptr;
		// Currently used for debug
		Shader* mp_plane_shader = nullptr;


		Framebuffer* mp_editor_pass_fb = nullptr; // Framebuffer that any editor stuff will be rendered into e.g grid
		Framebuffer* mp_picking_fb = nullptr;

		// Contains the actual rendering of the scene
		std::unique_ptr<Texture2D> mp_scene_display_texture{ nullptr };


		Events::EventListener<Events::WindowEvent> m_window_event_listener;

		std::unique_ptr<GridMesh> m_grid_mesh = nullptr;
		std::unique_ptr<Scene> m_active_scene = nullptr;

		std::unique_ptr<SceneEntity> mp_editor_camera{ nullptr };

		uint64_t m_mouse_selected_entity_id = 0;

		std::vector<uint64_t> m_selected_entity_ids;
		bool m_selected_entities_are_dragged = false;

		bool m_render_settings_window = false;



		// STYLING
		const float opacity = 1.f;
		const ImVec4 orange_color = ImVec4(0.9f, 0.2f, 0.05f, opacity);
		const ImVec4 orange_color_dark = ImVec4(0.5f, 0.1f, 0.025f, opacity);
		const ImVec4 orange_color_bright = ImVec4(0.9f, 0.2f, 0.0f, opacity);
		const ImVec4 orange_color_brightest = ImVec4(0.9f, 0.2f, 0.0f, opacity);
		const ImVec4 dark_grey_color = ImVec4(0.1f, 0.1f, 0.1f, opacity);
		const ImVec4 lighter_grey_color = ImVec4(0.2f, 0.2f, 0.2f, opacity);
		const ImVec4 lightest_grey_color = ImVec4(0.3f, 0.3f, 0.3f, opacity);
		inline static const float toolbar_height = 40;
		glm::vec2 file_explorer_window_size = { 750, 750 };
	};

#define ORNG_BASE_SCENE_YAML R"(Scene: Untitled scene
Entities:
  []
DirLight:
  Colour: [4.61000013, 4.92500019, 4.375]
  Direction: [0, 0.707106829, 0.707106829]
  CascadeRanges: [20, 75, 200]
  Zmults: [5, 5, 5]
Skybox:
  HDR filepath: ""
Bloom:
  Intensity: 1
  Knee: 0.100000001
  Threshold: 1)"
}
