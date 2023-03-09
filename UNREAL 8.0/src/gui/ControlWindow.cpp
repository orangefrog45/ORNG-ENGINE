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

const void ControlWindow::DisplayPointLightControls(unsigned int num_lights, LightConfigValues& light_values) {
	static std::vector<double> vals;
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