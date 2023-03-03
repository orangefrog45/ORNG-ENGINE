#include <stdio.h>
#include <util/util.h>
#include <ctype.h>
#include "InputHandle.h"

InputHandle::InputHandle() {
	wPressed = false;
	aPressed = false;
	ePressed = false;
	qPressed = false;
	sPressed = false;
	dPressed = false;
	shiftPressed = false;
	ctrlPressed = false;
}

void InputHandle::HandleInput(GLFWwindow* window, int window_width, int window_height)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		mouse_locked = true;
	}
	else if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
		mouse_locked = false;
	}

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		wPressed = true;
	}
	else {
		wPressed = false;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		sPressed = true;
	}
	else {
		sPressed = false;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		aPressed = true;
	}
	else {
		aPressed = false;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		dPressed = true;
	}
	else {
		dPressed = false;
	}
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		qPressed = true;
	}
	else {
		qPressed = false;
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		ePressed = true;
	}
	else {
		ePressed = false;
	}
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
		g_pressed = true;
	}
	else {
		g_pressed = false;
	}
	glfwGetCursorPos(window, &mouse_x, &mouse_y);

	if (mouse_locked) glfwSetCursorPos(window, window_width / 2, window_height / 2);


}


