#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include <vector>
#pragma once

struct LightConfigValues {
	float atten_constant = 1.0f;
	float atten_linear = 0.05f;
	float atten_exp = 0.01f;
	float max_distance = 48.0f;
	bool lights_enabled = 1.0f;
};

struct DebugConfigValues {
	bool depth_map_view = false;
};

struct DirectionalLightConfigValues {
	glm::vec3 light_color = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 light_position = glm::vec3(0.5f, 0.5f, 1.f);
};

struct ControlWindow {
	static void Render();
	static void CreateBaseWindow();
	static const void DisplayPointLightControls(unsigned int num_lights, LightConfigValues& light_values);
	static const void DisplayDebugControls(DebugConfigValues& config_values, unsigned int depth_map_texture);
	static const void DisplayDirectionalLightControls(DirectionalLightConfigValues& config_values);
};