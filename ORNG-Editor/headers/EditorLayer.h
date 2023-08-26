#pragma once
#include "EngineAPI.h"
#include "../extern/imgui/imgui.h"

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
		void OnProjectEvent(const Events::ProjectEvent& t_event);


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

		void CreateMeshPreview(MeshAsset* p_asset);
		void CreateMaterialPreview(const Material* p_material);

		void RenderErrorMessages();
		void GenerateErrorMessage(const std::string& error_str = "");
		void RenderProjectGenerator(int& selected_component_from_popup);

		// Renders material as a drag-drop target, returns ptr of the new material if a material was drag-dropped on it, else nullptr
		Material* RenderMaterialComponent(const Material* p_material);

		SceneEntity* DuplicateEntity(SceneEntity* p_original);

		void RenderSceneGraph();
		void RenderProfilingTimers();
		void RenderSkyboxEditor();

		enum EntityNodeEvent {

			E_NONE = 0,
			E_DELETE = 1,
			E_DUPLICATE = 2
		};

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


		void ShowAssetManager();
		void RenderTextureEditorSection();
		void RenderMaterialEditorSection();



		void ShowFileExplorer(const std::string& starting_path, wchar_t extension_filter[], std::function<void(std::string)> valid_file_callback);

		static bool ShowVec3Editor(const char* name, glm::vec3& vec, float min = std::numeric_limits<float>::lowest(), float max = std::numeric_limits<float>::max());
		static bool ShowVec2Editor(const char* name, glm::vec2& vec, float min = std::numeric_limits<float>::lowest(), float max = std::numeric_limits<float>::max());
		static bool ShowColorVec3Editor(const char* name, glm::vec3& vec);
		void RenderMaterialTexture(const char* name, Texture2D*& p_tex);
		static bool H1TreeNode(const char* name);
		static bool H2TreeNode(const char* name);
		static bool ClampedFloatInput(const char* name, float* p_val, float min = std::numeric_limits<float>::lowest(), float max = std::numeric_limits<float>::max());
		// Creates an empty imgui tree node
		static bool EmptyTreeNode(const char* name);

		// Texture spec for rendering the scene
		Texture2DSpec m_color_render_texture_spec;
		Texture2DSpec m_asset_preview_spec;

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
		std::unordered_map<const MeshAsset*, std::shared_ptr<Texture2D>> m_mesh_preview_textures;
		std::unordered_map<const Material*, std::shared_ptr<Texture2D>> m_material_preview_textures;
		// Have to store these in a vector and process at a specific stage otherwise ImGui breaks, any material in this vector will have a preview rendered for use in its imgui imagebutton
		std::vector<Material*> m_materials_to_gen_previews;

		// Used to render error messages - stores log history at point the error occured
		std::vector<std::vector<std::string>> m_error_log_stack;

		Events::EventListener<Events::ProjectEvent> m_asset_listener;

		Events::EventListener<Events::WindowEvent> m_window_event_listener;

		std::unique_ptr<GridMesh> m_grid_mesh = nullptr;
		std::unique_ptr<Scene> m_active_scene = nullptr;
		std::unique_ptr<Scene> mp_preview_scene = nullptr;

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
MeshAssets:
  []
TextureAssets:
  []
Materials:
  []
Entities:
  []
DirLight:
  Colour: [4.61000013, 4.92500019, 4.375]
  Direction: [0, 0.707106769, 0.707106769]
  CascadeRanges: [20, 75, 200]
  Zmults: [5, 5, 5]
Skybox:
  HDR filepath: ""
Bloom:
  Intensity: 1
  Knee: 0.100000001
  Threshold: 1)"
}

