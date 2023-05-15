#pragma once

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

		static unsigned int GetWidth() {
			return Get().m_window_width;
		}

		static unsigned int GetHeight() {
			return Get().m_window_height;
		}
	private:
		static Window& Get() {
			static Window s_instance;
			return s_instance;
		}

		void I_Init();
		GLFWwindow* p_window = nullptr;
		unsigned int m_window_width = 1920;
		unsigned int m_window_height = 1080;
	};

}