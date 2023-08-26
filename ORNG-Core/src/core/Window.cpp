#include "pch/pch.h"
#include <GLFW/glfw3.h>
#include "core/Window.h"
#include "events/EventManager.h"
#include "core/Input.h"

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


	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		Events::KeyEvent e_event;
		e_event.key = key;
		switch (action) {
		case GLFW_RELEASE:
			e_event.event_type = Input::InputType::RELEASE;
			break;
		case GLFW_PRESS:
			e_event.event_type = Input::InputType::PRESS;
			break;
		case GLFW_REPEAT:
			e_event.event_type = Input::InputType::PRESS;
			break;
		}
		Events::EventManager::DispatchEvent(e_event);
	}

	void Window::ISetCursorPos(int x, int y) { glfwSetCursorPos(p_window, x, y); }

	bool Window::I_IsKeyDown(int key) { return (glfwGetKey(p_window, std::toupper(key)) == GLFW_PRESS); }

	bool Window::I_IsMouseButtonInState(int key, unsigned int state) { return glfwGetMouseButton(p_window, key) == state; }

	glm::vec2 Window::IGetMousePos() {
		glfwGetCursorPos(Get().p_window, &mouse_x, &mouse_y);
		return glm::vec2(mouse_x, mouse_y);
	};

	bool Window::IsMouseButtonDown(int button) {
		return Get().I_IsMouseButtonInState(button, GLFW_PRESS);
	}

	bool Window::IsMouseButtonUp(int button) {
		return Get().I_IsMouseButtonInState(button, GLFW_RELEASE);
	}

	void Window::IUpdate() {
		m_scroll_data.active = false;
		m_scroll_data.offset = { 0,0 };
	}


	void Window::I_Init() {
		p_window = glfwCreateWindow(m_window_width, m_window_height, "ORANGE ENGINE", nullptr, nullptr);
		glfwSetScrollCallback(p_window, [](GLFWwindow* window, double xoffset, double yoffset) {Window::SetScrollActive(glm::vec2(xoffset, yoffset)); });

		glfwSetWindowSizeCallback(p_window, [](GLFWwindow*, int width, int height)
			{
				Window::SetWindowDimensions(width, height);
			}
		);


		if (!p_window)
		{
			glfwTerminate();
		}
		glfwSetKeyCallback(p_window, key_callback);

		glfwMakeContextCurrent(p_window);
		glfwSwapInterval(0);
		glfwSetInputMode(p_window, GLFW_STICKY_KEYS, GLFW_TRUE); // keys "stick" until they've been polled

	}
}