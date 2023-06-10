#include "pch/pch.h"

#include "components/CameraComponent.h"
#include "rendering/Renderer.h"
#include "core/Window.h"
#include "core/FrameTiming.h"

namespace ORNG {

	void CameraComponent::Update() {

		if (!is_active)
			return;

		UpdateFrustum();

	}


	void CameraComponent::UpdateFrustum() {

		const float half_far_plane_height = tanf(glm::radians(fov * 0.5f)) * zFar;
		const float half_far_plane_width = half_far_plane_height * (static_cast<float>(Window::GetWidth()) / static_cast<float>(Window::GetHeight()));

		const glm::vec3 normalized_target = target;
		right = glm::normalize(glm::cross(target, up));
		const glm::vec3 up = glm::cross(right, target);

		const glm::vec3 far_point = pos + normalized_target * zFar;

		view_frustum.near_plane = { normalized_target, pos + zNear * normalized_target };
		view_frustum.far_plane = { -normalized_target, far_point };

		const glm::vec3 point_right_plane = far_point + right * half_far_plane_width;
		view_frustum.right_plane = { glm::cross(up, point_right_plane - pos), pos };

		const glm::vec3 point_left_plane = far_point - right * half_far_plane_width;
		view_frustum.left_plane = { glm::cross(up, pos - point_left_plane), pos };

		const glm::vec3 point_up_plane = far_point + up * half_far_plane_height;
		view_frustum.top_plane = { glm::cross(right, pos - point_up_plane), pos };

		const glm::vec3 point_down_plane = far_point - up * half_far_plane_height;
		view_frustum.bottom_plane = { glm::cross(right, point_down_plane - pos), pos };
	}
	void CameraComponent::SetPosition(float x, float y, float z) {
		pos.x = x;
		pos.y = y;
		pos.z = z;
		UpdateFrustum();
	}


	void CameraComponent::MoveForward(float time_elapsed) {
		pos += target * speed * time_elapsed;
		UpdateFrustum();
	}
	void CameraComponent::MoveBackward(float time_elapsed) {
		pos -= target * speed * time_elapsed;
		UpdateFrustum();
	}
	void CameraComponent::StrafeLeft(float time_elapsed) {
		pos += -right * speed * time_elapsed;
		UpdateFrustum();
	}
	void CameraComponent::StrafeRight(float time_elapsed) {
		pos += right * speed * time_elapsed;
		UpdateFrustum();
	}
	void CameraComponent::MoveUp(float time_elapsed) {
		pos += speed * up * time_elapsed;
		UpdateFrustum();
	}
	void CameraComponent::MoveDown(float time_elapsed) {
		pos -= speed * up * time_elapsed;
		UpdateFrustum();
	}

	glm::mat4x4 CameraComponent::GetViewMatrix() const {
		return glm::lookAt(pos, pos + target, up);
	}

	glm::mat4x4 CameraComponent::GetProjectionMatrix() const {
		return glm::perspective(glm::radians(fov / 2.0f), static_cast<float>(Window::GetWidth()) / static_cast<float>(Window::GetHeight()), zNear, zFar);
	}

}