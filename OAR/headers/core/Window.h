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
			return Get().I_IsMouseButtonDown(button);
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


	private:
		void I_Init();

		glm::vec2 IGetMousePos() {
			glfwGetCursorPos(Get().p_window, &mouse_x, &mouse_y);
			return glm::vec2(mouse_x, mouse_y);
		};

		void ISetWindowDimensions(int width, int height);

		void ISetCursorPos(float x, float y) { glfwSetCursorPos(p_window, x, y); }

		bool I_IsKeyDown(int key) { return (glfwGetKey(p_window, std::toupper(key)) == GLFW_PRESS); }

		bool I_IsMouseButtonDown(int key) { return glfwGetMouseButton(p_window, key) == GLFW_PRESS; }

		void I_Init(GLFWwindow* ptr) { p_window = ptr; }

		double IGetTimeStep() const { return last_frame_time; }


		static Window& Get() {
			static Window s_instance;
			return s_instance;
		}

		GLFWwindow* p_window = nullptr;
		unsigned int last_frame_time = 0;
		double mouse_x = 0;
		double mouse_y = 0;
		unsigned int m_window_width = 1920;
		unsigned int m_window_height = 1080;
	};

}