#pragma once
#include "util/TimeStep.h"
#include "util/util.h"
#include "events/EventManager.h"

struct GLFWwindow;

namespace ORNG {

	class Window {
	public:
		static void Init(glm::ivec2 initial_dimensions, const char* name, int initial_window_display_monitor_idx, bool iconified, bool decorated) {
			Get().I_Init(initial_dimensions, name, initial_window_display_monitor_idx, iconified, decorated);
		};

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


		static ScrollData GetScrollStatus() {
			return Get().m_scroll_data;
		}
	private:
		void I_Init(glm::ivec2 initial_dimensions, const char* name, int initial_window_display_monitor_idx, bool iconified, bool decorated);

		void ISetWindowDimensions(int width, int height);

		void ISetCursorPos(int x, int y);

		void IUpdate();

		ScrollData m_scroll_data;

		// Handles events with event_type == SET to update window
		Events::EventListener<Events::MouseEvent> m_mouse_listener;


		static Window& Get() {
			static Window s_instance;
			return s_instance;
		}

		GLFWwindow* p_window = nullptr;
		unsigned int m_window_width = 0;
		unsigned int m_window_height = 0;
	};

}