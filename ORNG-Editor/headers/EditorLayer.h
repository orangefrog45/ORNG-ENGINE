#pragma once
#include "EngineAPI.h"
#include "../extern/imgui/imgui.h"
#include "AssetManagerWindow.h"

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

		enum EntityNodeEvent {

			E_NONE = 0,
			E_DELETE = 1,
			E_DUPLICATE = 2
		};

	private:
		void OnInit() override { Init(); };
		void Update() override;
		void OnRender() override { RenderDisplayWindow(); RenderUI(); };

		// Sets up directory structure for a new project
		bool GenerateProject(const std::string& project_name);
		// Sets working directory to folder_path so all resources will be found there.
		bool MakeProjectActive(const std::string& folder_path);
		void InitImGui();
		// Window containing the actual rendered scene
		void RenderDisplayWindow();
		void RenderToolbar();
		void RenderUI();
		// Movement
		void UpdateEditorCam();

		void RenderGrid();
		void DoPickingPass();
		/* Highlight the selected entities in the editor */
		void DoSelectedEntityHighlightPass();

		void BeginPlayScene();
		void EndPlayScene();


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
		void RenderScriptComponentEditor(ScriptComponent* p_script);
		void RenderAudioComponentEditor(AudioComponent* p_audio);

		void RenderErrorMessages();
		void GenerateErrorMessage(const std::string& error_str = "");
		void RenderProjectGenerator(int& selected_component_from_popup);

		enum ProjectErrFlags {
			NONE = 0,
			NO_SCENE_YML = 1 << 0,
			NO_ASSET_YML = 1 << 1,
			NO_RES_FOLDER = 1 << 2,
			NO_MESH_FOLDER = 1 << 3,
			NO_TEXTURE_FOLDER = 1 << 4,
			NO_SHADER_FOLDER = 1 << 5,
			NO_SCRIPT_FOLDER = 1 << 6,
			CORRUPTED_SCENE_YML = 1 << 7,
			CORRUPTED_ASSET_YML = 1 << 8,
		};



		// Ensures a project directory is in the correct state to be used in the editor, 
		ProjectErrFlags ValidateProjectDir(const std::string& dir_path);
		bool RepairProjectDir(const std::string& dir_path);

		// Renders material as a drag-drop target, returns ptr of the new material if a material was drag-dropped on it, else nullptr
		Material* RenderMaterialComponent(const Material* p_material);

		void RenderSceneGraph();
		void RenderProfilingTimers();
		void RenderSkyboxEditor();



		EntityNodeEvent RenderEntityNode(SceneEntity* p_entity, unsigned int layer);
		void RenderDirectionalLightEditor();
		void RenderGlobalFogEditor();
		void RenderBloomEditor();
		void RenderTerrainEditor();

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

		AssetManagerWindow m_asset_manager_window{ &m_current_project_directory };

		// Stores temporary serialized yaml data to load back in after exiting "play mode"
		std::string m_temp_scene_serialization;
		// If true editor will start simulating the scene as if it were running in a runtime layer
		bool m_play_mode_active = false;

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

		Framebuffer* mp_editor_pass_fb = nullptr; // Framebuffer that any editor stuff will be rendered into e.g grid
		Framebuffer* mp_picking_fb = nullptr;

		// Contains the actual rendering of the scene
		std::unique_ptr<Texture2D> mp_scene_display_texture{ nullptr };

		// Used to render error messages - stores log history at point the error occured
		std::vector<std::vector<std::string>> m_error_log_stack;


		Events::EventListener<Events::WindowEvent> m_window_event_listener;

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
		Texture2DSpec m_current_2d_tex_spec;
		Material* mp_selected_material = nullptr;


		// STYLING
		const float opacity = 1.f;
		const ImVec4 orange_color = ImVec4(0.9f, 0.2f, 0.05f, opacity);
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

#define ORNG_BASE_ASSET_YAML R"(TextureAssets:
  []
Materials:
  []
)"
}

