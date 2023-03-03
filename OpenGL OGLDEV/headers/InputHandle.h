#pragma once
#include <glew.h>
#include <glfw/glfw3.h>
struct InputHandle {
	bool wPressed = false;
	bool aPressed = false;
	bool sPressed = false;
	bool dPressed = false;
	bool ePressed = false;
	bool qPressed = false;
	bool g_pressed = false;
	bool shiftPressed = false;
	bool ctrlPressed = false;
	bool ESC_pressed = false;
	bool mouse_locked = false;
	float mouse_x = 0;
	float mouse_y = 0;
	InputHandle();
	void SetMouseLocked(bool t) { mouse_locked = t; };
	void HandleInput(GLFWwindow* window, int window_width, int window_height);
};

