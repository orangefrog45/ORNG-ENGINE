#include "InputHandle.h"
#include <glm/gtx/transform.hpp>


InputHandle::InputHandle() {
	w_down = false;
	a_down = false;
	e_down = false;
	q_down = false;
	s_down = false;
	d_down = false;
	shift_down = false;
	ctrl_down = false;
}

void InputHandle::HandleCameraInput(Camera& camera, InputHandle& input) {
	float time_interval = static_cast<float>(input.m_time_step.GetTimeInterval());

	if (input.w_down)
		camera.MoveForward(time_interval);

	if (input.a_down)
		camera.StrafeLeft(time_interval);

	if (input.s_down)
		camera.MoveBackward(time_interval);

	if (input.d_down)
		camera.StrafeRight(time_interval);

	if (input.e_down)
		camera.MoveUp(time_interval);

	if (input.q_down)
		camera.MoveDown(time_interval);

	/* Mouse handling */
	const float rotationSpeed = 0.005f;
	float maxDelta = 100.0f;

	if (input.mouse_locked) {
		auto mouseDelta = -glm::vec2(input.mouse_x - static_cast<double>(RendererResources::GetWindowWidth()) / 2, input.mouse_y - static_cast<double>(RendererResources::GetWindowHeight()) / 2);

		if (mouseDelta.x > -maxDelta && mouseDelta.x < maxDelta) {
			camera.m_target = glm::rotate(mouseDelta.x * rotationSpeed, camera.m_up) * glm::fvec4(camera.m_target, 0);
		}

		if (mouseDelta.y > -maxDelta && mouseDelta.y < maxDelta) {
			glm::fvec3 m_targetNew = glm::rotate(mouseDelta.y * rotationSpeed, glm::cross(camera.m_target, camera.m_up)) * glm::fvec4(camera.m_target, 0);
			//constraint to stop lookAt flipping from y axis alignment
			if (m_targetNew.y <= 0.999f && m_targetNew.y >= -0.999f) {
				camera.m_target = m_targetNew;
			}
		}
		glm::normalize(camera.m_target);

	}

	input.m_time_step.UpdateLastTime();
}

void InputHandle::HandleInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
	{
		mouse_locked = true;
	}
	else if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
		mouse_locked = false;
	}

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		w_down = true;
	}
	else {
		w_down = false;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		s_down = true;
	}
	else {
		s_down = false;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		a_down = true;
	}
	else {
		a_down = false;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		d_down = true;
	}
	else {
		d_down = false;
	}
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		q_down = true;
	}
	else {
		q_down = false;
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		e_down = true;
	}
	else {
		e_down = false;
	}
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
		g_down = true;
	}
	else {
		g_down = false;
	}
	glfwGetCursorPos(window, &mouse_x, &mouse_y);

	if (mouse_locked) glfwSetCursorPos(window, RendererResources::GetWindowWidth() / 2, RendererResources::GetWindowHeight() / 2);

}


