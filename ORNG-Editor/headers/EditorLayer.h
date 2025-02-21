#pragma once
#include "EngineAPI.h"
#include "../extern/imgui/imgui.h"
#include "ImGuizmo.h"
#include "AssetManagerWindow.h"
#include "scene/GridMesh.h"
#include "Settings.h"
#include "EditorEventStack.h"
#include "LuaCLI.h"
#include "rendering/RenderGraph.h"

namespace physx {
	class PxMaterial;
}
struct DragData {
	glm::ivec2 start{ 0, 0 };
	glm::ivec2 end{0, 0};
};

#define SCENE (*mp_scene_context)

namespace ORNG {

	enum EntityNodeEvent {
		E_NONE = 0,
		E_DELETE = 1,
		E_DUPLICATE = 2
	};

	struct EntityNodeData {
		EntityNodeData(EntityNodeEvent _event, ImVec2 _node_max, ImVec2 _node_min) : e_event(_event), node_screen_max(_node_max), node_screen_min(_node_min) {};
		EntityNodeEvent e_event;

		ImVec2 node_screen_max;
		ImVec2 node_screen_min;
	};

	enum class SelectionMode {
		ENTITY,
		JOINT
	};




	class EditorLayer : public Layer {
		friend class GameLayer;
	public:
		EditorLayer(Scene* p_scene, const std::string& start_filepath) : mp_scene_context(p_scene), m_start_filepath(start_filepath) { m_asset_manager_window.SetScene(p_scene); };

		void Init();

		void SetScene(Scene* p_scene);

	private:
		void OnInit() override { Init(); };

		void Update() override;

		void OnImGuiRender() override { RenderUI(); };

		void OnRender() override {
			RenderDisplayWindow(); 
			if (m_state.simulate_mode_active) mp_scene_context->OnRender();
		};

		void OnShutdown() override;

		/*
			Editor UI and rendering
		*/

		void InitRenderGraph();

		void InitImGui();

		// Window containing the actual rendered scene
		void RenderDisplayWindow();

		void RenderToolbar();

		void RenderUI();

		void RenderSceneDisplayPanel();

		// Renders a popup with shortcuts to create a new entity with a component, e.g mesh
		void RenderCreationWidget(SceneEntity* p_entity, bool trigger);

		EntityNodeData RenderEntityNode(SceneEntity* p_entity, unsigned int layer, bool node_selection_active, const Box2D& selection_box);

		struct EntityNodeEntry {
			SceneEntity* p_entity;

			// How far is this entity nested (how many parents does it have)
			unsigned depth;
		};


		// Outputs an ordered list of entities with their children depending on if their nodes are opened or not.
		void GetEntityGraph(std::vector<EntityNodeEntry>& output);
		void PushEntityIntoGraph(SceneEntity* p_entity, std::vector<EditorLayer::EntityNodeEntry>& output, unsigned depth);

		void DisplayEntityEditor();

		void RenderGeneralSettingsMenu();

		void RenderSceneGraph();

		Material* RenderMaterialComponent(const Material* p_material);

		void RenderGrid();

		void DoPickingPass();

		// Highlight the selected entities in the editor
		void DoSelectedEntityHighlightPass();

		void RenderPhysxDebug();

		void UpdateSceneDisplayRect();

		/*
			Editor tools
		*/

		void MultiSelectDisplay();

		void RenderJointMaker();

		void PollKeybinds();

		void RenderJointEditor(JointComponent::Joint* p_joint);

		void SelectEntity(uint64_t id);

		void DeselectEntity(uint64_t id);

		void InitLua();

		void UpdateLuaEntityArray();

		void BeginPlayScene();

		void EndPlayScene();

		/*
			Misc modifiers
		*/

		void RenderSkyboxEditor();

		void RenderDirectionalLightEditor();

		void RenderGlobalFogEditor();

		void RenderBloomEditor();

		/*
			Entity/Component modifiers		
		*/

		void DeleteEntitiesTracked(std::vector<uint64_t> entities); // Copy vector as this will usually be m_selected_entity_id's and deselecting entities will modify it, throwing off the range-based for loop

		std::vector<SceneEntity*> DuplicateEntitiesTracked(std::vector<uint64_t> entities); // Copy vector for same reason as above

		// Duplicates entity and conditionally calls OnCreate on it depending on if simulate mode is active
		SceneEntity& CreateEntityTracked(const std::string& name);

		void RenderMeshComponentEditor(MeshComponent* comp);

		void RenderMeshWithMaterials(const MeshAsset* p_asset, std::vector<const Material*>& materials, std::function<void(MeshAsset* p_new)> OnMeshDrop, std::function<void(unsigned index, Material* p_new)> OnMaterialDrop);

		void RenderPointlightEditor(PointLightComponent* light);

		void RenderSpotlightEditor(SpotLightComponent* light);

		void RenderCameraEditor(CameraComponent* p_cam);

		void RenderTransformComponentEditor(std::vector<TransformComponent*>& transforms);

		void RenderPhysicsComponentEditor(PhysicsComponent* p_comp);

		void RenderScriptComponentEditor(ScriptComponent* p_script);

		void RenderAudioComponentEditor(AudioComponent* p_audio);

		void RenderVehicleComponentEditor(VehicleComponent* p_comp);

		void RenderParticleEmitterComponentEditor(ParticleEmitterComponent* p_comp);

		void RenderParticleBufferComponentEditor(class ParticleBufferComponent* p_comp);

		void RenderCharacterControllerComponentEditor(CharacterControllerComponent* p_comp);

		void RenderJointComponentEditor(JointComponent* p_comp);

		void RenderEntityNodeRef(EntityNodeRef& ref);

		/*
			Project handling
		*/

		void RenderProjectGenerator(int& selected_component_from_popup);

		// Ensures a project directory is in the correct state to be used in the editor, attempts repairs if not
		bool ValidateProjectDir(const std::string& dir_path);

		// Sets up directory structure for a new project
		bool GenerateProject(const std::string& project_name, bool abs_path);

		// Sets working directory to folder_path so all resources will be found there.
		bool MakeProjectActive(const std::string& folder_path);

		void BuildGameFromActiveProject();

		/*
			Misc
		*/

		void UpdateEditorCam(); // Movement

		glm::vec2 ConvertFullscreenMouseToDisplayMouse(glm::vec2 mouse_coords);

		/*
			State/resources
		*/

		RenderGraph m_render_graph;
		Scene* mp_scene_context = nullptr;
		std::unique_ptr<SceneEntity> mp_editor_camera{ nullptr };

		struct EditorState {
			bool render_ui_in_simulation = false; // In simulation mode, determines if Editor UI will be rendered on top. Editor UI is always rendered in non-simulation mode, no matter this setting
			bool fullscreen_scene_display = false; // Whenever simulation mode is active this will be true

			bool simulate_mode_active = false; // If true editor will start simulating the scene as if it were running in a runtime layer
			bool simulate_mode_paused = false;

			bool selected_entities_are_dragged = false;
			bool item_selected_this_frame = false;

			std::vector<uint64_t> selected_entity_ids;
			std::vector<uint64_t> open_tree_nodes_entities;

			DragData mouse_drag_data;

			ImVec2 scene_display_rect{ 1, 1 }; // Size, not position

			std::string executable_directory;
			std::string current_project_directory; // Working directory will always be this

			std::string temp_scene_serialization; // Stores temporary serialized yaml data to load back in after exiting simulation mode

			SelectionMode selection_mode = SelectionMode::ENTITY;

			ImGuizmo::OPERATION current_gizmo_operation = ImGuizmo::TRANSLATE;
			ImGuizmo::MODE current_gizmo_mode = ImGuizmo::WORLD;

			JointComponent::Joint* p_selected_joint = nullptr;

			GeneralSettings general_settings;
		};

		struct EditorResources {
			Texture2DSpec color_render_texture_spec; // Texture spec for rendering the scene
			std::unique_ptr<Texture2D> p_scene_display_texture{ nullptr };

			std::unique_ptr<GridMesh> grid_mesh = nullptr;

			Shader picking_shader;
			Shader grid_shader;
			Shader highlight_shader;
			Shader quad_col_shader;
			Shader plane_shader;

			enum class RaymarchSV { CAPSULE, };
			ShaderVariants raymarch_shader;

			Framebuffer editor_pass_fb; // Framebuffer that any editor stuff will be rendered into e.g grid
			Framebuffer picking_fb;

			FullscreenTexture2D picking_tex;

			VAO line_vao;

			// STYLING
			ImFont* p_xl_font = nullptr;
			ImFont* p_l_font = nullptr;
			ImFont* p_m_font = nullptr;

			static constexpr float opacity = 1.f;
			static constexpr ImVec4 orange_color = ImVec4(0.9f, 0.2f, 0.05f, opacity);
			static constexpr ImVec4 orange_color_dark = ImVec4(0.5f, 0.1f, 0.025f, opacity);
			static constexpr ImVec4 orange_color_bright = ImVec4(0.9f, 0.2f, 0.0f, opacity);
			static constexpr ImVec4 orange_color_brightest = ImVec4(0.9f, 0.2f, 0.0f, opacity);
			static constexpr ImVec4 dark_grey_color = ImVec4(0.1f, 0.1f, 0.1f, opacity);
			static constexpr ImVec4 lighter_grey_color = ImVec4(0.2f, 0.2f, 0.2f, opacity);
			static constexpr ImVec4 lightest_grey_color = ImVec4(0.3f, 0.3f, 0.3f, opacity);
			static constexpr ImVec4 blue_col = ImVec4(0, 100, 255, 1);
			static constexpr float toolbar_height = 40;
			glm::vec2 file_explorer_window_size = { 750, 750 };
		};

		EditorState m_state;
		EditorResources m_res;

		EditorEventStack m_event_stack;
		LuaCLI m_lua_cli;
		AssetManagerWindow m_asset_manager_window{ &m_state.current_project_directory, mp_scene_context };

		Events::EventListener<Events::WindowEvent> m_window_event_listener;

		const std::string m_start_filepath;
	};

#define ORNG_BASE_SCENE_YAML R"(Scene: Untitled scene
Entities:
  []
DirLight:
  Shadows: true
  Colour: [4.61000013, 4.92500019, 4.375]
  Direction: [0, 0.707106829, 0.707106829]
  CascadeRanges: [20, 75, 200]
  Zmults: [5, 5, 5]
Skybox:
  HDR filepath: "res/textures/preview-sky.hdr"
  IBL: true
  Resolution: 2048
Fog:
  Density: 0.0
  Absorption: 0.00300000003
  Scattering: 0.0399999991
  Anisotropy: 0.508000016
  Colour: [0.300000012, 0.300000012, 0.400000006]
  Steps: 23
  Emission: 0
Bloom:
  Intensity: 0.333000004
  Knee: 0.100000001
  Threshold: 1)"
}

#undef SCENE