#include <glm/glm.hpp>
#include "ExtraMath.h"
#include "freeglut.h"
#include "Camera.h"

Camera::Camera() {
	m_pos = glm::fvec3(0.0f, 0.0f, 0.0f);
	m_target = glm::fvec3(0.0f, 0.0f, 1.0f);
	m_up = glm::fvec3(0.0f, 1.0f, 0.0f);
}

void Camera::SetPosition(float x, float y, float z) {
	m_pos.x = x;
	m_pos.y = y;
	m_pos.z = z;
}

void Camera::OnKeyboard(unsigned char key) {
	switch (key) {

	case GLUT_KEY_UP:
		m_pos += m_target * m_speed;
		break;

	case GLUT_KEY_DOWN:
		m_pos -= (m_target * m_speed);
		break;

	case GLUT_KEY_LEFT:
	{
		glm::fvec3 left = glm::cross(m_target, m_up);
		glm::normalize(left);
		left *= m_speed;
		m_pos += left;
	}
	break;
	
	case GLUT_KEY_RIGHT:
	{
		glm::fvec3 right = glm::cross(m_up, m_target);
		glm::normalize(right);
		right *= m_speed;
		m_pos += right;
	}
	break;

	case GLUT_KEY_PAGE_UP:
		m_pos.y += m_speed;
		break;

	case GLUT_KEY_PAGE_DOWN:
		m_pos.y -= m_speed;
		break;
	}
}

glm::fmat4x4 Camera::GetMatrix() {
	glm::fmat4x4 cameraTransform = ExtraMath::Init3DCameraTransform(m_pos, m_target, m_up);
	return cameraTransform;
}