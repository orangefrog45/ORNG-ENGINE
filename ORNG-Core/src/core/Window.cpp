#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "pch/pch.h"
#include "core/Window.h"
#include "events/EventManager.h"

using namespace ORNG;


void Window::ISetWindowDimensions(int width, int height) {
	if (width < 10 || height < 10) {
		return; // Too small
	}

	Events::WindowEvent window_event;
	window_event.event_type = Events::WindowEvent::WINDOW_RESIZE;
	window_event.old_window_size = glm::vec2(m_window_width, m_window_height);
	window_event.new_window_size = glm::vec2(width, height);
	m_window_width = width;
	m_window_height = height;

	glfwSetWindowSize(p_window, m_window_width, m_window_height);

	Events::EventManager::DispatchEvent(window_event);
}


static void key_callback([[maybe_unused]] GLFWwindow* window, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mods) {
	ASSERT(key >= 0);

	switch (action) {
	case GLFW_RELEASE:
		Events::EventManager::DispatchEvent(Events::KeyEvent{ static_cast<unsigned>(key), static_cast<uint8_t>(InputType::RELEASE) });
		break;
	case GLFW_PRESS:
		Events::EventManager::DispatchEvent(Events::KeyEvent{ static_cast<unsigned>(key), static_cast<uint8_t>(InputType::PRESS) });
		break;
	case GLFW_REPEAT:
		// "Hold" state tracked in input, not here
		return;
	}
}

static glm::ivec2 gs_mouse_coords;

static void MouseButtonCallback(GLFWwindow* window, int button, int action, [[maybe_unused]] int mods) {
	double posx, posy;
	glfwGetCursorPos(window, &posx, &posy);
	Events::MouseEvent e_event{ static_cast<MouseAction>(action), static_cast<MouseButton>(button), glm::ivec2(floor(posx), floor(posy)), gs_mouse_coords };

	Events::EventManager::DispatchEvent(e_event);

	gs_mouse_coords = e_event.mouse_pos_new;
}

static void CursorPosCallback([[maybe_unused]] GLFWwindow* window, double xPos, double yPos) {
	Events::MouseEvent e_event{ MOVE, MouseButton::NONE, glm::ivec2(floor(xPos), floor(yPos)), gs_mouse_coords };

	Events::EventManager::DispatchEvent(e_event);

	gs_mouse_coords = e_event.mouse_pos_new;
}

void Window::ISetCursorPos(int x, int y) { glfwSetCursorPos(p_window, x, y); gs_mouse_coords = { x, y }; }

void Window::IUpdate() {
	input.OnUpdate();
}

void Window::SetCursorStyle(CursorStyle style) {
	static GLFWcursor* cursors[static_cast<size_t>(CursorStyle::NUM_CURSORS)] = {0};

	if (!cursors[0]) {
		cursors[static_cast<unsigned>(CursorStyle::ARROW)] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
		cursors[static_cast<unsigned>(CursorStyle::I_BEAM)] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
		cursors[static_cast<unsigned>(CursorStyle::VRESIZE)] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
		cursors[static_cast<unsigned>(CursorStyle::HRESIZE)] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
		cursors[static_cast<unsigned>(CursorStyle::HAND)] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
	}

	glfwSetCursor(Get().p_window, cursors[static_cast<unsigned>(style)]);
}

void Window::SetCursorVisible(bool visible) {
	glfwSetInputMode(Get().p_window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
}

void Window::I_Init(glm::ivec2 initial_dimensions, const char* name, int initial_window_display_monitor_idx, bool iconified, bool decorated, bool maximized) {
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

	if (maximized)
		glfwWindowHint(GLFW_MAXIMIZED, true);

	if (!decorated)
		glfwWindowHint(GLFW_DECORATED, false);

	if (!iconified)
		glfwWindowHint(GLFW_AUTO_ICONIFY, false);

	int count;
	GLFWmonitor** monitors = glfwGetMonitors(&count);
	if (initial_window_display_monitor_idx == -1) {
		p_window = glfwCreateWindow(initial_dimensions.x, initial_dimensions.y, name, nullptr, nullptr);
	}
	else if (initial_window_display_monitor_idx < count) {
		p_window = glfwCreateWindow(initial_dimensions.x, initial_dimensions.y, name, monitors[initial_window_display_monitor_idx], nullptr);
	}
	else {
		ORNG_CORE_ERROR("Initial window display monitor index too large, total monitors '{0}', received index '{1}'", count, initial_window_display_monitor_idx);
		p_window = glfwCreateWindow(initial_dimensions.x, initial_dimensions.y, name, nullptr, nullptr);
	}
	glfwSetScrollCallback(p_window, []([[maybe_unused]] GLFWwindow* window, double xoffset, double yoffset) {
		auto& input =  Window::Get().input;
		input.m_scroll_data.active = true;
		input.m_scroll_data.offset = {xoffset, yoffset};
	});
	glfwSetMouseButtonCallback(p_window, MouseButtonCallback);
	glfwSetCursorPosCallback(p_window, CursorPosCallback);
	glfwSetWindowSizeCallback(p_window, [](GLFWwindow*, int width, int height)
		{
			Window::SetWindowDimensions(width, height);
		}
	);

	if (!p_window)
		glfwTerminate();

	// Read window dimensions back as GLFW window creation sometimes doesn't follow what was set perfectly (especially with maximization)
	int width, height;
	glfwGetWindowSize(p_window, &width, &height);
	m_window_width = width;
	m_window_height = height;

	glfwSetKeyCallback(p_window, key_callback);
	glfwMakeContextCurrent(p_window);
	glfwSwapInterval(0);
	glfwSetInputMode(p_window, GLFW_STICKY_KEYS, GLFW_TRUE); // keys "stick" until they've been polled

	glClearColor(0.0f, 0.0f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glfwSwapBuffers(p_window);

	input.Init();
}

void Window::Shutdown() {
	glfwDestroyWindow(mp_instance->p_window);
	if (mp_instance) delete mp_instance;
}
