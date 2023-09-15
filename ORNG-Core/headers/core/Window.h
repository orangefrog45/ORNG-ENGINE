#pragma once
#include "util/TimeStep.h"
#include "util/util.h"

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

		static bool IsKeyDown(int key) {
			return Get().I_IsKeyDown(key);
		}

		static bool IsMouseButtonDown(int button);

		static bool IsMouseButtonUp(int button);


		static glm::vec2 GetMousePos() {
			return Get().IGetMousePos();
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

		glm::vec2 IGetMousePos();

		void ISetWindowDimensions(int width, int height);

		void ISetCursorPos(int x, int y);

		bool I_IsKeyDown(int key);

		bool I_IsMouseButtonInState(int key, unsigned int state);

		void IUpdate();

		ScrollData m_scroll_data;



		static Window& Get() {
			static Window s_instance;
			return s_instance;
		}

		GLFWwindow* p_window = nullptr;
		double mouse_x = 0;
		double mouse_y = 0;
		unsigned int m_window_width = 2560;
		unsigned int m_window_height = 1440;
	};

}