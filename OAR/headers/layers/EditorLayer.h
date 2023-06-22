#pragma once
#include "components/CameraComponent.h"
#include "scene/Scene.h"
#include "fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD.h"
#include "scene/GridMesh.h"
#include "events/EventManager.h"
#include "components/EditorCamera.h"


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

		void RenderDisplayWindow();
		void RenderUI();
		void Update();

		void RenderGrid();
		void DoPickingPass();
		void DoSelectedEntityHighlightPass();

		void DisplayEntityEditor();
		void RenderMeshComponentEditor(MeshComponent* comp);
		void RenderPointlightEditor(PointLightComponent* light);
		void RenderSpotlightEditor(SpotLightComponent* light);
		void RenderCameraEditor(CameraComponent* p_cam);

		void RenderSceneGraph();
		void RenderDirectionalLightEditor();
		void RenderGlobalFogEditor();
		void RenderTerrainEditor();


		void ShowAssetManager();
		void RenderTextureEditor(Texture2D* selected_texture, Texture2DSpec& spec);


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


	};
}