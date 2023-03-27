#include <glm/gtx/transform.hpp>
#include "ExtraMath.h"
#include "Camera.h"
#include "TimeStep.h"


Camera::Camera(int windowWidth, int windowHeight) : m_window_width(windowWidth), m_window_height(windowHeight) {
}

void Camera::SetPosition(float x, float y, float z) {
	m_pos.x = x;
	m_pos.y = y;
	m_pos.z = z;
}

glm::fvec3 Camera::GetPos() const {
	return m_pos;
}


void Camera::MoveForward(float time_elapsed) {
	m_pos += m_target * m_speed * time_elapsed;
}
void Camera::MoveBackward(float time_elapsed) {
	m_pos -= m_target * m_speed * time_elapsed;
}
void Camera::StrafeLeft(float time_elapsed) {
	glm::fvec3 left = glm::normalize(glm::cross(m_up, m_target));
	m_pos += left * m_speed * time_elapsed;
}
void Camera::StrafeRight(float time_elapsed) {
	glm::fvec3 right = glm::normalize(glm::cross(m_target, m_up));
	m_pos += right * m_speed * time_elapsed;
}
void Camera::MoveUp(float time_elapsed) {
	m_pos += m_speed * m_up * time_elapsed;
}
void Camera::MoveDown(float time_elapsed) {
	m_pos -= m_speed * m_up * time_elapsed;
}

glm::fmat4x4 Camera::GetMatrix() const {
	return glm::lookAt(m_pos, m_pos + m_target, m_up);
}

