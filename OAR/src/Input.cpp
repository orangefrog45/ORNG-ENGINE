#include "pch/pch.h"

#include "Input.h"

glm::vec2 Input::IGetMousePos() {
	glfwGetCursorPos(Get().p_window, &mouse_x, &mouse_y);
	return glm::vec2(mouse_x, mouse_y);
}


