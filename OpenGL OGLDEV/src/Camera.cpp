#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_major_storage.hpp>
#include "ExtraMath.h"
#include "freeglut.h"
#include <iostream>
#include "Camera.h"
#include "KeyboardState.h"
#include "TimeStep.h"

/*Camera::Camera(const glm::fvec3& Pos, const glm::fvec3& Target, const glm::fvec3& Up) {

	m_windowWidth = WindowWidth;
	m_windowHeight = WindowHeight;

	m_pos = Pos;
	m_target = Target;
	glm::normalize(m_target);
	m_up = Up;
	glm::normalize(m_up);

}*/

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
	if (keyboardState.shiftPressed == true) {
		MoveUp();
	};
	if (keyboardState.ctrlPressed == true) {
		MoveDown();
	};
}

void Camera::OnMouse(const glm::vec2& newMousePos) {
	const float rotationSpeed = 0.005f;
	glm::vec2 mouseDelta = glm::vec2(newMousePos.x-m_windowWidth/2, newMousePos.y-m_windowHeight/2);
	if (mouseDelta.x > -10.0f && mouseDelta.x < 10.0f ) {
	m_target = glm::rotate(mouseDelta.x * rotationSpeed, m_up) * glm::fvec4(m_target, 0);
	}
	if (mouseDelta.y > -10.0f && mouseDelta.y < 10.0f) {
	m_target = glm::rotate(mouseDelta.y * rotationSpeed, glm::cross(m_target, m_up)) * glm::fvec4(m_target, 0);
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

	glm::fmat4x4 cameraTransMatrix(
		1.0f, 0.0f, 0.0f, -m_pos.x,
		0.0f, 1.0f, 0.0f, -m_pos.y,
		0.0f, 0.0f, 1.0f, -m_pos.z,
		0.0f, 0.0f, 0.0f, 1.0f
	);


	//glm::fmat4x4 cameraTransform = ExtraMath::Init3DCameraTransform(m_pos, m_target, m_up);
	//return cameraTransform;
	return glm::rowMajor4(glm::lookAt(m_pos, m_pos + m_target, m_up));;
}

