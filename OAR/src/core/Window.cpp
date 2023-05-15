#include "pch/pch.h"
#include <GLFW/glfw3.h>
#include "core/Window.h"

namespace ORNG {
	void Window::I_Init() {
		p_window = glfwCreateWindow(m_window_width, m_window_height, "ORANGE ENGINE", nullptr, nullptr);

		if (!p_window)
		{
			glfwTerminate();
		}
	}
}