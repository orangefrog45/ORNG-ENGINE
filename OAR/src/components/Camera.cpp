#include "pch/pch.h"

#include <glm/gtx/transform.hpp>
#include <glm/vec4.hpp>
#include "components/Camera.h"
#include "Input.h"
#include "rendering/Renderer.h"

void Camera::Update() {
	if (Input::IsKeyDown('W'))
		MoveForward(Input::GetTimeStep());

	if (Input::IsKeyDown('A'))
		StrafeLeft(Input::GetTimeStep());

	if (Input::IsKeyDown('S'))
		MoveBackward(Input::GetTimeStep());

	if (Input::IsKeyDown('D'))
		StrafeRight(Input::GetTimeStep());

	if (Input::IsKeyDown('Q'))
		MoveDown(Input::GetTimeStep());

	if (Input::IsKeyDown('E'))
		MoveUp(Input::GetTimeStep());

	if (Input::IsKeyDown('R'))
		m_mouse_locked = true;

	if (Input::IsKeyDown('T'))
		m_mouse_locked = false;



	if (!m_mouse_locked) {
		glm::vec2 mouse_coords = Input::GetMousePos();
		float rotation_speed = 0.005f;
		auto mouseDelta = -glm::vec2(mouse_coords.x - static_cast<double>(Renderer::GetWindowWidth()) / 2, mouse_coords.y - static_cast<double>(Renderer::GetWindowHeight()) / 2);

		m_target = glm::rotate(mouseDelta.x * rotation_speed, m_up) * glm::vec4(m_target, 0);
		UpdateFrustum();

		glm::fvec3 m_targetNew = glm::rotate(mouseDelta.y * rotation_speed, glm::cross(m_target, m_up)) * glm::vec4(m_target, 0);
		//constraint to stop lookAt flipping from y axis alignment
		if (m_targetNew.y <= 0.9996 && m_targetNew.y >= -0.996) {
			m_target = m_targetNew;
		}
		glm::normalize(m_target);

		Input::SetCursorPos(Renderer::GetWindowWidth() / 2, Renderer::GetWindowHeight() / 2);
	}

	UpdateFrustum();

}


void Camera::UpdateFrustum() {

	const float half_far_plane_height = tanf(glm::radians(m_fov * 0.5f)) * m_zFar;
	const float half_far_plane_width = half_far_plane_height * (static_cast<float>(Renderer::GetWindowWidth()) / static_cast<float>(Renderer::GetWindowHeight()));

	const glm::vec3 normalized_target = m_target;
	m_right = glm::normalize(glm::cross(m_target, m_up));
	const glm::vec3 up = glm::cross(m_right, m_target);

	const glm::vec3 far_point = m_pos + normalized_target * m_zFar;

	m_view_frustum.near_plane = { normalized_target, m_pos + m_zNear * normalized_target };
	m_view_frustum.far_plane = { -normalized_target, far_point };

	const glm::vec3 point_right_plane = far_point + m_right * half_far_plane_width;
	m_view_frustum.right_plane = { glm::cross(up, point_right_plane - m_pos), m_pos };

	const glm::vec3 point_left_plane = far_point - m_right * half_far_plane_width;
	m_view_frustum.left_plane = { glm::cross(up, m_pos - point_left_plane), m_pos };

	const glm::vec3 point_up_plane = far_point + up * half_far_plane_height;
	m_view_frustum.top_plane = { glm::cross(m_right, m_pos - point_up_plane), m_pos };

	const glm::vec3 point_down_plane = far_point - up * half_far_plane_height;
	m_view_frustum.bottom_plane = { glm::cross(m_right, point_down_plane - m_pos), m_pos };
}
void Camera::SetPosition(float x, float y, float z) {
	m_pos.x = x;
	m_pos.y = y;
	m_pos.z = z;
	UpdateFrustum();
}

glm::vec3 Camera::GetPos() const {
	return m_pos;
}


void Camera::MoveForward(float time_elapsed) {
	m_pos += m_target * m_speed * time_elapsed;
	UpdateFrustum();
}
void Camera::MoveBackward(float time_elapsed) {
	m_pos -= m_target * m_speed * time_elapsed;
	UpdateFrustum();
}
void Camera::StrafeLeft(float time_elapsed) {
	m_pos += -m_right * m_speed * time_elapsed;
	UpdateFrustum();
}
void Camera::StrafeRight(float time_elapsed) {
	m_pos += m_right * m_speed * time_elapsed;
	UpdateFrustum();
}
void Camera::MoveUp(float time_elapsed) {
	m_pos += m_speed * m_up * time_elapsed;
	UpdateFrustum();
}
void Camera::MoveDown(float time_elapsed) {
	m_pos -= m_speed * m_up * time_elapsed;
	UpdateFrustum();
}

glm::mat4x4 Camera::GetViewMatrix() const {
	return glm::lookAt(m_pos, m_pos + m_target, m_up);
}

glm::mat4x4 Camera::GetProjectionMatrix() const {
	return glm::perspective(m_fov / 2.0f, static_cast<float>(Renderer::GetWindowWidth()) / static_cast<float>(Renderer::GetWindowHeight()), m_zNear, m_zFar);
}

