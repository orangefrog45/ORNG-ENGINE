#pragma once
#include <glew.h>
#include <glfw/glfw3.h>
#include "TimeStep.h"
#include "Camera.h"
class InputHandle {
public:
	friend class Application;
	InputHandle();
	void SetMouseLocked(bool t) { mouse_locked = t; };
	static void HandleCameraInput(Camera& camera, InputHandle& input);
	void HandleInput(GLFWwindow* window);

private:
	TimeStep m_time_step = TimeStep(TimeStep::TimeUnits::MICROSECONDS);
	bool w_down = false;
	bool a_down = false;
	bool s_down = false;
	bool d_down = false;
	bool e_down = false;
	bool q_down = false;
	bool g_down = false;
	bool shift_down = false;
	bool ctrl_down = false;
	bool esc_down = false;
	bool mouse_locked = false;
	double mouse_x = 0;
	double mouse_y = 0;
};

