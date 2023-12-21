#include "pch/pch.h"

#include "components/CameraComponent.h"
#include "core/Window.h"
#include "scene/SceneEntity.h"

namespace ORNG {
	inline static constexpr glm::vec3 CAM_TARGET_VEC{ 0.0, 0.0, -1.0 };
	void CameraComponent::Update() {
		UpdateFrustum();
	}

	/* TODO - MOVE TO CAMERASYSTEM */
	void CameraComponent::UpdateFrustum() {
		const float half_far_plane_height = tanf(glm::radians(fov * 0.5f)) * zFar;
		const float half_far_plane_width = half_far_plane_height * aspect_ratio;
		auto* p_transform = GetEntity()->GetComponent<TransformComponent>();
		auto transforms = p_transform->GetAbsoluteTransforms();
		glm::vec3 target = p_transform->forward;

		const glm::vec3 up = p_transform->up;
		const glm::vec3 right = p_transform->right;

		glm::vec3 pos = transforms[0];

		// Middle of zfar plane
		const glm::vec3 far_point = pos + target * zFar;

		view_frustum.near_plane = { target , pos + zNear * target };
		view_frustum.far_plane = { -target , far_point };

		const glm::vec3 point_right_plane = far_point + right * half_far_plane_width;
		view_frustum.right_plane = { glm::cross(up, point_right_plane - pos), pos };

		const glm::vec3 point_left_plane = far_point - right * half_far_plane_width;
		view_frustum.left_plane = { glm::cross(up, pos - point_left_plane), pos };

		const glm::vec3 point_up_plane = far_point + up * half_far_plane_height;
		view_frustum.top_plane = { glm::cross(right, pos - point_up_plane), pos };

		const glm::vec3 point_down_plane = far_point - up * half_far_plane_height;
		view_frustum.bottom_plane = { glm::cross(right, point_down_plane - pos), pos };
	}

	void CameraComponent::MakeActive() {
		is_active = true;
		Events::ECS_Event<CameraComponent> update_event{ Events::ECS_EventType::COMP_UPDATED, this };
		Events::EventManager::DispatchEvent(update_event);
	};


	glm::mat4x4 CameraComponent::GetProjectionMatrix() const {
		return glm::perspective(glm::radians(fov / 2.0f), aspect_ratio, zNear, zFar);
	}
}