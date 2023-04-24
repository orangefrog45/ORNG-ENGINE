#pragma once
#include "TimeStep.h"
#include "glfw/glfw3.h"

class Input {
public:
	static void UpdateTimeStep() {
		Get().IUpdateTimeStep();
	}

	static double GetTimeStep() {
		return Get().IGetTimeStep();
	}

	static Input& Get() {
		static Input s_instance;
		return s_instance;
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
	glm::vec2 IGetMousePos();
	void ISetCursorPos(float x, float y) { glfwSetCursorPos(p_window, x, y); }
	bool I_IsKeyDown(int key) { return (glfwGetKey(p_window, std::toupper(key)) == GLFW_PRESS); }
	bool I_IsMouseButtonDown(int key) { return glfwGetMouseButton(p_window, key) == GLFW_PRESS; }
	void I_Init(GLFWwindow* ptr) { p_window = ptr; }
	void IUpdateTimeStep() { last_frame_time = m_time_step.GetTimeInterval();  m_time_step.UpdateLastTime(); }
	double IGetTimeStep() { return last_frame_time; }

	GLFWwindow* p_window = nullptr;
	unsigned int last_frame_time = 0;
	TimeStep m_time_step = TimeStep(TimeStep::TimeUnits::MILLISECONDS);
	bool mouse_locked = false;
	double mouse_x = 0;
	double mouse_y = 0;
};

