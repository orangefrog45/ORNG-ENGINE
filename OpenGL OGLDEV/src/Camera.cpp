#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_major_storage.hpp>
#include "ExtraMath.h"
#include "freeglut.h"
#include <iostream>
#include "Camera.h"
#include "KeyboardState.h"
#include "TimeStep.h"


Camera::Camera(int windowWidth, int windowHeight, const TimeStep* time_step) : m_windowWidth(windowWidth), m_windowHeight(windowHeight), time_step(time_step) {
}

void Camera::SetPosition(float x, float y, float z) {
	m_pos.x = x;
	m_pos.y = y;
	m_pos.z = z;
}

glm::fvec3 Camera::GetPos() const {
	return m_pos;
}

void Camera::HandleInput(const KeyboardState& keyboard_state) {

	if (keyboard_state.wPressed)
		MoveForward();

	if (keyboard_state.aPressed)
		StrafeLeft();

	if (keyboard_state.sPressed)
		MoveBackward();

	if (keyboard_state.dPressed)
		StrafeRight();

	if (keyboard_state.ePressed)
		MoveUp();

	if (keyboard_state.qPressed)
		MoveDown();
}

void Camera::OnMouse(const glm::vec2& newMousePos) {
	const float rotationSpeed = 0.005f;
	float maxDelta = 250.0f;

	glm::vec2 mouseDelta = glm::vec2(newMousePos.x - m_windowWidth / 2, newMousePos.y - m_windowHeight / 2);
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
	glutWarpPointer(m_windowWidth / 2, m_windowHeight / 2);
}


void Camera::MoveForward() {
	m_pos -= m_target * m_speed * (float)time_step->timeInterval;
}
void Camera::MoveBackward() {
	m_pos += m_target * m_speed * (float)time_step->timeInterval;
}
void Camera::StrafeLeft() {
	glm::fvec3 left = glm::normalize(glm::cross(m_up, m_target));
	m_pos += left * m_speed * (float)time_step->timeInterval;
}
void Camera::StrafeRight() {
	glm::fvec3 right = glm::normalize(glm::cross(m_target, m_up));
	m_pos += right * m_speed * (float)time_step->timeInterval;
}
void Camera::MoveUp() {
	m_pos += m_speed * m_up * (float)time_step->timeInterval;
}
void Camera::MoveDown() {
	m_pos -= m_speed * m_up * (float)time_step->timeInterval;
}

glm::fmat4x4 Camera::GetMatrix() const {
	return glm::rowMajor4(glm::lookAt(m_pos, m_pos + m_target, m_up));
}

