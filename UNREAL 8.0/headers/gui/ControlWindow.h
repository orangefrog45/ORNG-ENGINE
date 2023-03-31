#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include <vector>
#pragma once



struct ControlWindow {
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
	static void Render();
	static void CreateBaseWindow();
	static  void DisplaySceneData(SceneData& data);
	static  void DisplayPointLightControls(unsigned int num_lights, LightConfigData& light_values);
	static  void DisplayDebugControls(DebugConfigData& config_values, unsigned int depth_map_texture);
	static  void DisplayDirectionalLightControls(DirectionalLightData& config_values);
	static void DisplayTerrainConfigControls(TerrainConfigData& config_values);
};