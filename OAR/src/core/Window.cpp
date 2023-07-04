#include "pch/pch.h"
#include <GLFW/glfw3.h>
#include "core/Window.h"
#include "events/EventManager.h"

namespace ORNG {

	void Window::ISetWindowDimensions(int width, int height) {

		if (width <= 750 || height <= 750) {
			// too small
			glfwSetWindowSize(p_window, m_window_width, m_window_height);
			return;
		}

		Events::WindowEvent window_event;
		window_event.event_type = Events::Event::WINDOW_RESIZE;
		window_event.old_window_size = glm::vec2(m_window_width, m_window_height);
		window_event.new_window_size = glm::vec2(width, height);
		m_window_width = width;
		m_window_height = height;

		Events::EventManager::DispatchEvent(window_event);
	}

	void Window::I_Init() {
		p_window = glfwCreateWindow(m_window_width, m_window_height, "ORANGE ENGINE", nullptr, nullptr);

		glfwSetWindowSizeCallback(p_window, [](GLFWwindow*, int width, int height)
			{
				Window::SetWindowDimensions(width, height);
			}
		);


		if (!p_window)
		{
			glfwTerminate();
		}
	}
}