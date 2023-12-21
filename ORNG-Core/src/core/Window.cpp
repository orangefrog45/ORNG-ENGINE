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


	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		Events::KeyEvent e_event;
		e_event.key = key;
		switch (action) {
		case GLFW_RELEASE:
			e_event.event_type = InputType::RELEASE;
			break;
		case GLFW_PRESS:
			e_event.event_type = InputType::PRESS;
			break;
		case GLFW_REPEAT:
			// "Hold" state tracked in input, not here
			return;
		}
		Events::EventManager::DispatchEvent(e_event);
	}

	static glm::ivec2 gs_mouse_coords;

	void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
		Events::MouseEvent e_event;
		e_event.event_type = Events::MouseEventType::RECEIVE;
		e_event.mouse_action = static_cast<MouseAction>(action);
		e_event.mouse_button = static_cast<MouseButton>(button);
		double posx, posy;
		glfwGetCursorPos(window, &posx, &posy);
		e_event.mouse_pos_old = gs_mouse_coords;
		e_event.mouse_pos_new = glm::ivec2(floor(posx), floor(posy));

		Events::EventManager::DispatchEvent(e_event);

		gs_mouse_coords = e_event.mouse_pos_new;
	}

	void CursorPosCallback(GLFWwindow* window, double xPos, double yPos) {
		Events::MouseEvent e_event;
		e_event.event_type = Events::MouseEventType::RECEIVE;
		e_event.mouse_action = MOVE;
		e_event.mouse_button = MouseButton::NONE;
		e_event.mouse_pos_old = gs_mouse_coords;
		e_event.mouse_pos_new = glm::ivec2(floor(xPos), floor(yPos));

		Events::EventManager::DispatchEvent(e_event);

		gs_mouse_coords = e_event.mouse_pos_new;
	}

	void Window::ISetCursorPos(int x, int y) { glfwSetCursorPos(p_window, x, y); gs_mouse_coords = { x, y }; }


	void Window::IUpdate() {
		m_scroll_data.active = false;
		m_scroll_data.offset = { 0,0 };
	}


	void Window::I_Init() {
		if (!glfwInit()) {
			ORNG_CORE_CRITICAL("GLFW Failed to initialize");
			exit(1);
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_SAMPLES, 4);
		glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
		glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

		p_window = glfwCreateWindow(m_window_width, m_window_height, "ORANGE ENGINE", nullptr, nullptr);
		glfwSetScrollCallback(p_window, [](GLFWwindow* window, double xoffset, double yoffset) {Window::SetScrollActive(glm::vec2(xoffset, yoffset)); });
		glfwSetMouseButtonCallback(p_window, MouseButtonCallback);
		glfwSetCursorPosCallback(p_window, CursorPosCallback);
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
		m_mouse_listener.OnEvent = [this](const Events::MouseEvent& t_event) {
			if (t_event.event_type != Events::MouseEventType::SET)
				return;

			if (t_event.mouse_action == MOVE)
				SetCursorPos(t_event.mouse_pos_new.x, t_event.mouse_pos_new.y);
			else if (t_event.mouse_action == TOGGLE_VISIBILITY) {
				glfwSetInputMode(p_window, GLFW_CURSOR, (std::any_cast<bool>(t_event.data_payload) ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED));
				ORNG_CORE_TRACE("TOGGLE VISIBLITY {0}", std::any_cast<bool>(t_event.data_payload));
			}

			};
		Events::EventManager::RegisterListener(m_mouse_listener);
	}
}