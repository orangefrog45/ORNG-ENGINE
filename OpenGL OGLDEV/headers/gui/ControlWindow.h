#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#pragma once

struct ControlWindow {
	static void Render();
	static void CreateBaseWindow();
	static glm::fvec3& DisplayAttenuationControls();
};