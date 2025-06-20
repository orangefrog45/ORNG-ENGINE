#pragma once
#include "core/Input.h"
#include "events/EventManager.h"
#include "GLFW/glfw3.h"
#include "util/TimeStep.h"
#include "util/util.h"

struct GLFWwindow;

namespace ORNG {
	class Window {
	public:
		static void InitInstance(Window* p_instance = nullptr) {
			mp_instance = p_instance ? p_instance : new Window();
		}

		static void Init(glm::ivec2 initial_dimensions, const char* name, int initial_window_display_monitor_idx, bool iconified, bool decorated, bool maximized) {
			Get().I_Init(initial_dimensions, name, initial_window_display_monitor_idx, iconified, decorated, maximized);
		};

		static void Shutdown() {
			glfwDestroyWindow(mp_instance->p_window);
			if (mp_instance) delete mp_instance;
		}

		static Window& Get() {
			DEBUG_ASSERT(mp_instance);
			return *mp_instance;
		}

		static GLFWwindow* GetGLFWwindow() {
			return Get().p_window;
		}

		static void SetWindowDimensions(int width, int height) {
			Get().ISetWindowDimensions(width, height);
		}

		static unsigned int GetWidth() {
			return Get().m_window_width;
		}

		static unsigned int GetHeight() {
			return Get().m_window_height;
		}

		static void SetCursorPos(int x, int y) {
			Get().ISetCursorPos(x, y);
		}

		static void SetCursorVisible(bool visible);

		static void SetScrollActive(glm::vec2 offset) {
			Get().m_scroll_data.active = true;
			Get().m_scroll_data.offset = offset;
		}

		static void Update() {
			Get().IUpdate();
		}

		struct ScrollData {
			bool active = false;
			glm::vec2 offset = { 0, 0 };
		};

		inline static Input& GetInput() noexcept {
			return Get().input;
		}

		Input input;
	private:
		void I_Init(glm::ivec2 initial_dimensions, const char* name, int initial_window_display_monitor_idx, bool iconified, bool decorated, bool maximized);

		void ISetWindowDimensions(int width, int height);

		void ISetCursorPos(int x, int y);

		void IUpdate();

		ScrollData m_scroll_data;

		inline static Window* mp_instance = nullptr;

		GLFWwindow* p_window = nullptr;
		unsigned int m_window_width = 0;
		unsigned int m_window_height = 0;

	};

}