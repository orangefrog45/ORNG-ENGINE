#pragma once
#include "scene/Scene.h"
#include "scene/GridMesh.h"
#include "events/EventManager.h"
#include "components/EditorCamera.h"
#include <imgui/imgui.h>

namespace physx {
	class PxMaterial;
}

namespace ORNG {
	class Shader;
	class Framebuffer;
	class Renderer;

	class EditorLayer {
	public:
		EditorLayer() {
			m_core_event_listener.OnEvent = [this](const Events::EngineCoreEvent& t_event) {

				switch (t_event.event_type) {

				case Events::Event::ENGINE_RENDER:
					RenderDisplayWindow();
					RenderUI();
					break;
				case Events::Event::ENGINE_UPDATE:
					Update();
					break;
				}
			};

			Events::EventManager::RegisterListener(m_core_event_listener);
		}

		void Init();

	private:
		void InitImGui();

		void RenderDisplayWindow();
		void RenderUI();
		void Update();

		void RenderEditorWindow();

		void RenderGrid();
		void DoPickingPass();
		void DoSelectedEntityHighlightPass();

		void RenderCreationWidget(SceneEntity* p_entity, bool trigger);
		void DisplayEntityEditor();
		void RenderMeshComponentEditor(MeshComponent* comp);
		void RenderPointlightEditor(PointLightComponent* light);
		void RenderSpotlightEditor(SpotLightComponent* light);
		void RenderCameraEditor(CameraComponent* p_cam);
		void RenderTransformComponentEditor(std::vector<TransformComponent*>& transforms);
		void RenderPhysicsComponentEditor(PhysicsComponent* p_comp);
		void RenderPhysicsMaterial(physx::PxMaterial* p_material);

		SceneEntity* DuplicateEntity(SceneEntity* p_original);

		void RenderSceneGraph();
		// Returns true if entity has been flagged for deletion
		void RenderEntityNode(SceneEntity* p_entity);
		void RenderDirectionalLightEditor();
		void RenderGlobalFogEditor();
		void RenderTerrainEditor();


		void ShowAssetManager();
		void RenderTextureEditorSection();
		void RenderMaterialEditorSection();



		/// Basic file explorer, success callback called when a file is clicked that has a valid extension, fail callback called when file has invalid extension.
		/// Always provide extensions fully capitalized.
		void ShowFileExplorer(std::string& path_ref, std::string& entry_ref, const std::vector<std::string>& valid_extensions, std::function<void()> valid_file_callback, std::function<void()> invalid_file_callback);

		static bool ShowVec3Editor(const char* name, glm::vec3& vec, float min = std::numeric_limits<float>::lowest(), float max = std::numeric_limits<float>::max());
		static bool ShowVec2Editor(const char* name, glm::vec2& vec, float min = std::numeric_limits<float>::lowest(), float max = std::numeric_limits<float>::max());
		static bool ShowColorVec3Editor(const char* name, glm::vec3& vec);
		void RenderMaterialTexture(const char* name, Texture2D*& p_tex);
		static bool H1TreeNode(const char* name);
		static bool H2TreeNode(const char* name);
		static bool ClampedFloatInput(const char* name, float* p_val, float min = std::numeric_limits<float>::lowest(), float max = std::numeric_limits<float>::max());


		Events::EventListener<Events::EngineCoreEvent> m_core_event_listener;

		Shader* mp_quad_shader = nullptr;
		Shader* mp_picking_shader = nullptr;
		Shader* mp_grid_shader = nullptr;
		Shader* mp_highlight_shader = nullptr;

		Framebuffer* mp_editor_pass_fb = nullptr; // Framebuffer that any editor stuff will be rendered into e.g grid
		Framebuffer* mp_picking_fb = nullptr;

		std::unique_ptr<GridMesh> m_grid_mesh = nullptr;
		std::unique_ptr<Scene> m_active_scene = nullptr;

		std::unique_ptr<EditorCamera> mp_editor_camera{ nullptr };

		uint64_t m_mouse_selected_entity_id = 0;

		std::vector<uint64_t> m_selected_entity_ids;
		bool m_selected_entities_are_dragged = false;

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