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

std::vector<double>& ControlWindow::DisplayPointLightControls(unsigned int num_lights) {
	static float atten_constant = 1.0f;
	static float atten_linear = 0.05f;
	static float atten_exp = 0.01f;
	static float max_distance = 48.0f;
	static bool lights_enabled = 1.0f;
	static std::vector<double> vals;
	ImGui::Text("POINTLIGHT CONTROLS");
	ImGui::SliderFloat("constant", &atten_constant, 0.0f, 1.0f);
	ImGui::SliderFloat("linear", &atten_linear, 0.0f, 1.0f);
	ImGui::SliderFloat("exp", &atten_exp, 0.0f, 0.1f);
	ImGui::SliderFloat("max distance", &max_distance, 0.0f, 200.0f);
	ImGui::Text(std::format("Pointlights: {}", num_lights).c_str());
	ImGui::Checkbox("Toggle Pointlights", &lights_enabled);

	vals = std::vector<double>{ atten_constant, atten_linear, atten_exp, max_distance, static_cast<double>(lights_enabled) };
	return vals;
}

void ControlWindow::Render() {
	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}