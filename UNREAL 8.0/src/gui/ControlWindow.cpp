#include <format>
#include "gui/ControlWindow.h"

void ControlWindow::CreateBaseWindow() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	{
		ImGui::Begin("Tools");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	}

}

void ControlWindow::DisplayTerrainConfigControls(TerrainConfigData& config_values) {
	ImGui::Text("TERRAIN CONTROLS");
	ImGui::InputInt("Seed", &config_values.seed);
	ImGui::SliderInt("Resolution", &config_values.resolution, 1, 100);
	ImGui::SliderFloat("Noise sampling resolution", &config_values.sampling_resolution, 1, 200);
	ImGui::SliderFloat("Height scale", &config_values.height_scale, -100, 100);
}

void ControlWindow::DisplayDebugControls(DebugConfigData& config_values, unsigned int depth_map_texture) {
	ImGui::Text("DEBUG CONTROLS");
	ImGui::Checkbox("Toggle depth view", &config_values.depth_map_view);

	if (config_values.depth_map_view == true) {
		ImGui::End();
		ImGui::Begin("DEPTH MAP");
		ImGui::Image((void*)depth_map_texture, ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
		ImGui::End();
		ImGui::Begin("Tools");
	}
}

void ControlWindow::DisplayDirectionalLightControls(DirectionalLightData& config_values) {

	ImGui::Text("DIR LIGHT CONTROLS");
	ImGui::SliderFloat("x-pos", &config_values.light_position.x, -1.0f, 1.0f);
	ImGui::SliderFloat("y-pos", &config_values.light_position.y, -1.0f, 1.0f);
	ImGui::SliderFloat("z-pos", &config_values.light_position.z, -1.0f, 1.0f);
	ImGui::SliderFloat("r", &config_values.light_color.x, 0.0f, 1.0f);
	ImGui::SliderFloat("g", &config_values.light_color.y, 0.0f, 1.0f);
	ImGui::SliderFloat("b", &config_values.light_color.z, 0.0f, 1.0f);
}


void ControlWindow::DisplayPointLightControls(unsigned int num_lights, LightConfigData& light_values) {
	ImGui::Text("POINTLIGHT CONTROLS");
	ImGui::SliderFloat("constant", &light_values.atten_constant, 0.0f, 1.0f);
	ImGui::SliderFloat("linear", &light_values.atten_linear, 0.0f, 1.0f);
	ImGui::SliderFloat("exp", &light_values.atten_exp, 0.0f, 0.1f);
	ImGui::SliderFloat("max distance", &light_values.max_distance, 0.0f, 200.0f);
	ImGui::Text(std::format("Pointlights: {}", num_lights).c_str());
	ImGui::Checkbox("Toggle Pointlights", &light_values.lights_enabled);
}

void ControlWindow::DisplaySceneData(SceneData& data) {
	ImGui::Text("SCENE INFO");
	ImGui::Text(std::format("VERTEX COUNT: {}", data.total_vertices).c_str());
	ImGui::Text(std::format("LIGHT COUNT: {}", data.num_lights).c_str());
};

void ControlWindow::Render() {
	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}