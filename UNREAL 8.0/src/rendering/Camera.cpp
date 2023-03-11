#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_major_storage.hpp>
#include <iostream>
#include <util/util.h>
#include "ExtraMath.h"
#include "Camera.h"
#include "TimeStep.h"


Camera::Camera(int windowWidth, int windowHeight, const std::shared_ptr<InputHandle> keyboard) : m_windowWidth(windowWidth), m_windowHeight(windowHeight), time_step(time_step),
input_handle(keyboard) {
}

void Camera::SetPosition(float x, float y, float z) {
	m_pos.x = x;
	m_pos.y = y;
	m_pos.z = z;
}

glm::fvec3 Camera::GetPos() const {
	return m_pos;
}

void Camera::HandleInput() {

	time_step.timeInterval = glfwGetTime() - time_step.lastTime;
	time_step.lastTime = glfwGetTime();

	if (input_handle->wPressed)
		MoveForward();

	if (input_handle->aPressed)
		StrafeLeft();

	if (input_handle->sPressed)
		MoveBackward();

	if (input_handle->dPressed)
		StrafeRight();

	if (input_handle->ePressed)
		MoveUp();

	if (input_handle->qPressed)
		MoveDown();

	OnMouse(static_cast<float>(input_handle->mouse_x), static_cast<float>(input_handle->mouse_y));
}

void Camera::OnMouse(float mouse_x, float mouse_y) {
	const float rotationSpeed = 0.005f;
	float maxDelta = 100.0f;

	if (input_handle->mouse_locked) {
		auto mouseDelta = -glm::vec2(mouse_x - static_cast<float>(m_windowWidth) / 2, mouse_y - static_cast<float>(m_windowHeight) / 2);

		if (mouseDelta.x > -maxDelta && mouseDelta.x < maxDelta) {
			m_target = glm::rotate(mouseDelta.x * rotationSpeed, m_up) * glm::fvec4(m_target, 0);
		}

		if (mouseDelta.y > -maxDelta && mouseDelta.y < maxDelta) {
			glm::fvec3 m_targetNew = glm::rotate(mouseDelta.y * rotationSpeed, glm::cross(m_target, m_up)) * glm::fvec4(m_target, 0);
			//constraint to stop lookAt flipping from y axis alignment
			if (m_targetNew.y <= 0.999f && m_targetNew.y >= -0.999f) {
				m_target = m_targetNew;
			}
		}
		glm::normalize(m_target);

	}
}


void Camera::MoveForward() {
	m_pos += m_target * m_speed * static_cast<float>(time_step.timeInterval);
}
void Camera::MoveBackward() {
	m_pos -= m_target * m_speed * static_cast<float>(time_step.timeInterval);
}
void Camera::StrafeLeft() {
	glm::fvec3 left = glm::normalize(glm::cross(m_up, m_target));
	m_pos += left * m_speed * static_cast<float>(time_step.timeInterval);
}
void Camera::StrafeRight() {
	glm::fvec3 right = glm::normalize(glm::cross(m_target, m_up));
	m_pos += right * m_speed * static_cast<float>(time_step.timeInterval);
}
void Camera::MoveUp() {
	m_pos += m_speed * m_up * static_cast<float>(time_step.timeInterval);
}
void Camera::MoveDown() {
	m_pos -= m_speed * m_up * static_cast<float>(time_step.timeInterval);
}

glm::fmat4x4 Camera::GetMatrix() const {
	return glm::rowMajor4(glm::lookAt(m_pos, m_pos + m_target, m_up));
}

