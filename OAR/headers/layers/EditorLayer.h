#pragma once
#include "scene/Scene.h"
#include "scene/GridMesh.h"
#include "events/EventManager.h"
#include "components/EditorCamera.h"
#include <imgui/imgui.h>



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

		void DisplayEntityEditor();
		void RenderMeshComponentEditor(MeshComponent* comp);
		void RenderPointlightEditor(PointLightComponent* light);
		void RenderSpotlightEditor(SpotLightComponent* light);
		void RenderCameraEditor(CameraComponent* p_cam);
		void RenderTransformComponentEditor(TransformComponent* p_transform);

		void RenderSceneGraph();
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
		static bool ShowColorVec3Editor(const char* name, glm::vec3& vec);
		void RenderMaterialTexture(const char* name, Texture2D*& p_tex);
		static bool H1TreeNode(const char* name);
		static bool H2TreeNode(const char* name);


		Events::EventListener<Events::EngineCoreEvent> m_core_event_listener;

		Shader* mp_quad_shader = nullptr;
		Shader* mp_picking_shader = nullptr;
		Shader* mp_grid_shader = nullptr;
		Shader* mp_highlight_shader = nullptr;

		Framebuffer* mp_editor_pass_fb = nullptr; // Framebuffer that any editor stuff will be rendered into e.g grid
		Framebuffer* mp_picking_fb = nullptr;

		std::unique_ptr<GridMesh> m_grid_mesh = nullptr;
		std::unique_ptr<Scene> m_active_scene = nullptr;

		EditorCamera m_editor_camera{ nullptr };

		unsigned int m_selected_entity_id = 0;

		struct DisplayWindowSettings {
			bool depth_map_view = false;
		};


		struct PointLightConfigData {
			float atten_constant;
			float atten_linear;
			float atten_exp;
			float max_distance;
			glm::vec3 color;
			glm::vec3 pos;
		};

		struct SpotLightConfigData {
			PointLightConfigData base;
			float aperture;
			glm::vec3 direction;
		};

		struct DirectionalLightData {
			glm::vec3 light_color = glm::vec3(1.0f, 1.0f, 1.0f);
			glm::vec3 light_direction = glm::vec3(0.5f, 0.5f, 1.f);
		};

		struct SceneData {
			unsigned int total_vertices;
			unsigned int num_lights;
		};

		struct MeshComponentData {
			glm::vec3 scale;
			glm::vec3 rotation;
			glm::vec3 position;
			int material_id;
		};

		// UI VARS
		Texture2D* mp_selected_texture = nullptr;
		Texture2D* mp_dragged_texture = nullptr;
		Texture2DSpec m_current_2d_tex_spec;
		Material* mp_selected_material = nullptr;
		Material* mp_dragged_material = nullptr;


		// STYLING
		const float opacity = 1.f;
		const ImVec4 orange_color = ImVec4(0.9, 0.2, 0.05, opacity);
		const ImVec4 orange_color_bright = ImVec4(0.9, 0.2, 0.0, opacity);
		const ImVec4 orange_color_brightest = ImVec4(0.9, 0.2, 0.0, opacity);
		const ImVec4 dark_grey_color = ImVec4(0.1, 0.1, 0.1, opacity);
		const ImVec4 lighter_grey_color = ImVec4(0.2, 0.2, 0.2, opacity);
		const ImVec4 lightest_grey_color = ImVec4(0.3, 0.3, 0.3, opacity);
		glm::vec2 file_explorer_window_size = { 750, 750 };
	};
}