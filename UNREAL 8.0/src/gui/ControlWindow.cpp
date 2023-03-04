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

glm::fvec3& ControlWindow::DisplayAttenuationControls() {
	static float atten_constant = 1.0f;
	static float atten_linear = 0.05f;
	static float atten_exp = 0.01f;
	ImGui::Text("ATTENUATION CONTROLS");
	ImGui::SliderFloat("constant", &atten_constant, 0.0f, 1.0f);
	ImGui::SliderFloat("linear", &atten_linear, 0.0f, 1.0f);
	ImGui::SliderFloat("exp", &atten_exp, 0.0f, 1.0f);

	glm::fvec3 val = glm::fvec3(atten_constant, atten_linear, atten_exp);
	return val;
}

void ControlWindow::Render() {
	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}