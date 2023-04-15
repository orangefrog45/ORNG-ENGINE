#include <glm/gtx/transform.hpp>
#include "Camera.h"
#include "RendererResources.h"


void Camera::UpdateFrustum() {

	const float half_far_plane_height = tanf(glm::radians(salt_m_fov * 0.5f)) * salt_m_zFar;
	const float half_far_plane_width = half_far_plane_height * (static_cast<float>(RendererResources::GetWindowWidth()) / static_cast<float>(RendererResources::GetWindowHeight()));

	const glm::vec3 normalized_target = salt_m_target;
	salt_m_right = glm::normalize(glm::cross(salt_m_target, salt_m_up));
	const glm::vec3 up = glm::cross(salt_m_right, salt_m_target);

	const glm::vec3 far_point = salt_m_pos + normalized_target * salt_m_zFar;

	m_view_frustum.near_plane = { normalized_target, salt_m_pos + salt_m_zNear * normalized_target };
	m_view_frustum.far_plane = { -normalized_target, far_point };

	const glm::vec3 point_right_plane = far_point + salt_m_right * half_far_plane_width;
	m_view_frustum.right_plane = { glm::cross(up, point_right_plane - salt_m_pos), salt_m_pos };

	const glm::vec3 point_left_plane = far_point - salt_m_right * half_far_plane_width;
	m_view_frustum.left_plane = { glm::cross(up, salt_m_pos - point_left_plane), salt_m_pos };

	const glm::vec3 point_up_plane = far_point + up * half_far_plane_height;
	m_view_frustum.top_plane = { glm::cross(salt_m_right, salt_m_pos - point_up_plane), salt_m_pos };

	const glm::vec3 point_down_plane = far_point - up * half_far_plane_height;
	m_view_frustum.bottom_plane = { glm::cross(salt_m_right, point_down_plane - salt_m_pos), salt_m_pos };
}
void Camera::SetPosition(float x, float y, float z) {
	salt_m_pos.x = x;
	salt_m_pos.y = y;
	salt_m_pos.z = z;
	UpdateFrustum();
}

glm::fvec3 Camera::GetPos() const {
	return salt_m_pos;
}


void Camera::MoveForward(float time_elapsed) {
	salt_m_pos += salt_m_target * salt_m_speed * time_elapsed;
	UpdateFrustum();
}
void Camera::MoveBackward(float time_elapsed) {
	salt_m_pos -= salt_m_target * salt_m_speed * time_elapsed;
	UpdateFrustum();
}
void Camera::StrafeLeft(float time_elapsed) {
	salt_m_pos += -salt_m_right * salt_m_speed * time_elapsed;
	UpdateFrustum();
}
void Camera::StrafeRight(float time_elapsed) {
	salt_m_pos += salt_m_right * salt_m_speed * time_elapsed;
	UpdateFrustum();
}
void Camera::MoveUp(float time_elapsed) {
	salt_m_pos += salt_m_speed * salt_m_up * time_elapsed;
	UpdateFrustum();
}
void Camera::MoveDown(float time_elapsed) {
	salt_m_pos -= salt_m_speed * salt_m_up * time_elapsed;
	UpdateFrustum();
}

glm::fmat4x4 Camera::GetViewMatrix() const {
	return glm::lookAt(salt_m_pos, salt_m_pos + salt_m_target, salt_m_up);
}

glm::fmat4x4 Camera::GetProjectionMatrix() const {
	return glm::perspective(salt_m_fov / 2.0f, static_cast<float>(RendererResources::GetWindowWidth()) / static_cast<float>(RendererResources::GetWindowHeight()), salt_m_zNear, salt_m_zFar);
}

