#pragma once
#include "util/TimeStep.h"
#include "GLFW/glfw3.h"


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

		static bool IsMouseButtonDown(int button) {
			return Get().I_IsMouseButtonInState(button, GLFW_PRESS);
		}

		static bool IsMouseButtonUp(int button) {
			return Get().I_IsMouseButtonInState(button, GLFW_RELEASE);
		}

		static void Init(GLFWwindow* ptr) {
			Get().I_Init(ptr);
		}

		static glm::vec2 GetMousePos() {
			return Get().IGetMousePos();
		}

		static void SetCursorPos(float x, float y) {
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

		glm::vec2 IGetMousePos() {
			glfwGetCursorPos(Get().p_window, &mouse_x, &mouse_y);
			return glm::vec2(mouse_x, mouse_y);
		};

		void ISetWindowDimensions(int width, int height);

		void ISetCursorPos(float x, float y) { glfwSetCursorPos(p_window, x, y); }

		bool I_IsKeyDown(int key) { return (glfwGetKey(p_window, std::toupper(key)) == GLFW_PRESS); }

		bool I_IsMouseButtonInState(int key, unsigned int state) { return glfwGetMouseButton(p_window, key) == state; }

		void I_Init(GLFWwindow* ptr) { p_window = ptr; }
#
		void IUpdate();

		ScrollData m_scroll_data;



		static Window& Get() {
			static Window s_instance;
			return s_instance;
		}

		GLFWwindow* p_window = nullptr;
		double mouse_x = 0;
		double mouse_y = 0;
		unsigned int m_window_width = 1920;
		unsigned int m_window_height = 1080;
	};

}