#pragma once
#include <glm/vec3.hpp>
#include <vector>
#include <memory>
#include "Quad.h"

class Scene;
class Renderer;

class EditorLayer {
public:
	EditorLayer(Renderer* renderer) : mp_renderer(renderer) {};
	void Init();
	void ShowDisplayWindow();
	void ShowUIWindow();
private:
	Renderer* mp_renderer = nullptr;

	struct LightConfigData {
		float atten_constant = 1.0f;
		float atten_linear = 0.05f;
		float atten_exp = 0.01f;
		float max_distance = 48.0f;
		bool lights_enabled = 1.0f;
	};

	struct TerrainConfigData {
		bool operator==(const TerrainConfigData& other) {
			if (!(height_scale == other.height_scale)
				|| !(seed == other.seed)
				|| !(resolution == other.resolution)
				|| !(sampling_density == other.sampling_density)) {
				return false;
			}
			else {
				return true;
			}
		};
		int seed = 123;
		int resolution = 10;
		float sampling_density = 20.0f;
		float height_scale = 10.0f;
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

	void CreateBaseWindow();
	void DisplaySceneData();
	void DisplayPointLightControls();
	void DisplayDebugControls(unsigned int depth_map_texture);
	void DisplayDirectionalLightControls();
	void DisplayTerrainConfigControls();

	SceneData m_scene_data;
	DirectionalLightData m_dir_light_data;
	DebugConfigData m_debug_config_data;
	TerrainConfigData m_terrain_config_data;
	LightConfigData m_light_config_data;
	Quad m_display_quad;
	std::unique_ptr<Scene> m_active_scene = std::make_unique<Scene>();
};