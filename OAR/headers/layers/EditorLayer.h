#pragma once
#include "rendering//Quad.h"
#include "components/Camera.h"
#include "scene/Scene.h"
#include "fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD.h"

namespace ORNG {

	class Renderer;

	class EditorLayer {
	public:
		friend class Application;
		EditorLayer() = default;
		void Init();
		void ShowDisplayWindow();
		void ShowUIWindow();
		void Update();


		struct NoiseSettings {
			float frequency = 0.1f;
			int seed = 123;
			float cutoff = 0.2f;
			FastNoiseSIMD::NoiseType type = FastNoiseSIMD::Cellular;
		};
		NoiseSettings m_noise_settings;

	private:

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

		struct DebugConfigData {
			bool depth_map_view = false;
		};

		struct DirectionalLightData {
			glm::vec3 light_color = glm::vec3(1.0f, 1.0f, 1.0f);
			glm::vec3 light_position = glm::vec3(0.5f, 0.5f, 1.f);
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

		void DisplayEntityEditor();
		void CreateBaseWindow();
		void DisplaySceneData();
		void EnablePointLightControls(PointLightComponent* light);
		void EnableMeshComponentControls(MeshComponent* comp);
		void EnableDebugControls(unsigned int depth_map_texture);
		void EnableDirectionalLightControls();
		void DisplayFogHeader();
		void DisplaySceneEntities();
		void EnableSpotLightControls(SpotLightComponent* light);
		void ShowAssetManager();


		SceneData m_scene_data;
		DirectionalLightData m_dir_light_data;
		DebugConfigData m_debug_config_data;
		PointLightConfigData m_pointlight_config_data;
		SpotLightConfigData m_spotlight_config_data;
		Camera m_editor_camera;
		MeshComponentData m_selected_mesh_data;
		std::unique_ptr<Scene> m_active_scene = nullptr;
	};
}