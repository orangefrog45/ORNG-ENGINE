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

const void ControlWindow::DisplayDebugControls(DebugConfigValues& config_values, unsigned int depth_map_texture) {
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

const void ControlWindow::DisplayDirectionalLightControls(DirectionalLightConfigValues& config_values) {

	ImGui::Text("DIR LIGHT CONTROLS");
	ImGui::SliderFloat("x-pos", &config_values.light_position.x, -1.0f, 1.0f);
	ImGui::SliderFloat("y-pos", &config_values.light_position.y, -1.0f, 1.0f);
	ImGui::SliderFloat("z-pos", &config_values.light_position.z, -1.0f, 1.0f);
	ImGui::ColorPicker4("color", &config_values.light_color[0]);

}


const void ControlWindow::DisplayPointLightControls(unsigned int num_lights, LightConfigValues& light_values) {
	ImGui::Text("POINTLIGHT CONTROLS");
	ImGui::SliderFloat("constant", &light_values.atten_constant, 0.0f, 1.0f);
	ImGui::SliderFloat("linear", &light_values.atten_linear, 0.0f, 1.0f);
	ImGui::SliderFloat("exp", &light_values.atten_exp, 0.0f, 0.1f);
	ImGui::SliderFloat("max distance", &light_values.max_distance, 0.0f, 200.0f);
	ImGui::Text(std::format("Pointlights: {}", num_lights).c_str());
	ImGui::Checkbox("Toggle Pointlights", &light_values.lights_enabled);
}

void ControlWindow::Render() {
	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}