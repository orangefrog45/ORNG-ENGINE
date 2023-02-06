#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_major_storage.hpp>
#include "ExtraMath.h"
#include "freeglut.h"
#include <iostream>
#include "Camera.h"
#include "KeyboardState.h"
#include "TimeStep.h"


Camera::Camera(int windowWidth, int windowHeight, TimeStep* timeStep) : m_windowWidth(windowWidth), m_windowHeight(windowHeight), timeStep(timeStep), m_speed(0.001f) {
	m_target = glm::fvec3(0.0f, 0.0f, -1.0f);
	m_pos = glm::fvec3(0.0f, 0.0f, 0.0f);
	m_up = glm::fvec3(0.0f, 1.0f, 0.0f);
}

void Camera::SetPosition(float x, float y, float z) {
	m_pos.x = x;
	m_pos.y = y;
	m_pos.z = z;
}

void Camera::HandleInput(KeyboardState* keyboard) {

	KeyboardState keyboardState = *keyboard;
	if (keyboardState.wPressed == true) {
		MoveForward();
	};
	if (keyboardState.aPressed == true) {
		StrafeLeft();
	};
	if (keyboardState.sPressed == true) {
		MoveBackward();
	};
	if (keyboardState.dPressed == true) {
		StrafeRight();
	};
	if (keyboardState.ePressed == true) {
		MoveUp();
	};
	if (keyboardState.qPressed == true) {
		MoveDown();
	};
}

void Camera::OnMouse(const glm::vec2& newMousePos) {
	const float rotationSpeed = 0.005f;
	float maxDelta = 50.0f;

	glm::vec2 mouseDelta = glm::vec2(newMousePos.x-m_windowWidth/2, newMousePos.y-m_windowHeight/2);
	if (mouseDelta.x > -maxDelta && mouseDelta.x < maxDelta ) {
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
	glutWarpPointer(m_windowWidth/2, m_windowHeight/2);
}


void Camera::MoveForward() {
	m_pos -= m_target * m_speed * (float)timeStep->timeInterval;
}
void Camera::MoveBackward() {
	m_pos += m_target * m_speed * (float)timeStep->timeInterval;
}
void Camera::StrafeLeft() {
	glm::fvec3 left = glm::normalize(glm::cross(m_up, m_target));
	m_pos += left * m_speed * (float)timeStep->timeInterval;
}
void Camera::StrafeRight() {
	glm::fvec3 right = glm::normalize(glm::cross(m_target, m_up));
	m_pos += right * m_speed * (float)timeStep->timeInterval;
}
void Camera::MoveUp() {
	m_pos += m_speed * m_up * (float)timeStep->timeInterval;
}
void Camera::MoveDown() {
	m_pos -= m_speed * m_up * (float)timeStep->timeInterval;
}

glm::fmat4x4 Camera::GetMatrix() {
	return glm::rowMajor4(glm::lookAt(m_pos, m_pos + m_target, m_up));;
}

