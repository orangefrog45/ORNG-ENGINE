#pragma once
#include "util/TimeStep.h"
#include "util/util.h"
#include "events/EventManager.h"

class GLFWwindow;
namespace ORNG {

	class Window {
	public:
		static void Init() {
			Get().I_Init();
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
			bool active;
			glm::vec2 offset;
		};


		static ScrollData GetScrollStatus() {
			return Get().m_scroll_data;
		}
	private:
		void I_Init();

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
		unsigned int m_window_width = 2560;
		unsigned int m_window_height = 1440;
	};

}