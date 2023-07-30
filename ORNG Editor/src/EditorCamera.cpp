#include "pch/pch.h"
#include "EditorCamera.h"
#include "core/Window.h"
#include "core/FrameTiming.h"
#include "scene/SceneEntity.h"

namespace ORNG {

	void EditorCamera::Update() {
		if (Window::IsKeyDown('W')) {
			MoveForward((float)FrameTiming::GetTimeStep());
		}

		if (Window::IsKeyDown('A'))
			StrafeLeft((float)FrameTiming::GetTimeStep());

		if (Window::IsKeyDown('S'))
			MoveBackward((float)FrameTiming::GetTimeStep());

		if (Window::IsKeyDown('D'))
			StrafeRight((float)FrameTiming::GetTimeStep());

		if (Window::IsKeyDown('Q'))
			MoveDown((float)FrameTiming::GetTimeStep());

		if (Window::IsKeyDown('E'))
			MoveUp((float)FrameTiming::GetTimeStep());

		if (Window::IsKeyDown('R')) {
			mouse_locked = true;
			Window::SetCursorPos(Window::GetWidth() / 2, Window::GetHeight() / 2);
		}

		auto scroll_data = Window::GetScrollStatus();
		if (scroll_data.active && speed > 0.0001 && speed < 1.0) {
			speed *= scroll_data.offset.y < 0.0 ? 0.9 : 1.1;
		}

		if (Window::IsKeyDown('T')) {
			mouse_locked = false;
			Window::SetCursorPos(Window::GetWidth() / 2, Window::GetHeight() / 2);
		}



		if (!mouse_locked) {
			glm::vec2 mouse_coords = Window::GetMousePos();
			float rotation_speed = 0.005f;
			glm::vec2 mouse_delta = -glm::vec2(mouse_coords.x - Window::GetWidth() / 2, mouse_coords.y - Window::GetHeight() / 2);

			target = glm::rotate(mouse_delta.x * rotation_speed, up) * glm::vec4(target, 0);

			glm::fvec3 target_new = glm::rotate(mouse_delta.y * rotation_speed, glm::cross(target, up)) * glm::vec4(target, 0);
			//constraint to stop lookAt flipping from y axis alignment
			if (target_new.y <= 0.9996f && target_new.y >= -0.996f) {
				target = target_new;
			}
			target = glm::normalize(target);

			Window::SetCursorPos(Window::GetWidth() / 2, Window::GetHeight() / 2);
		}

		UpdateFrustum();
	}



	void EditorCamera::MoveForward(float time_elapsed) {
		auto& transform = *GetEntity()->GetComponent<TransformComponent>();
		glm::vec3 pos = transform.GetAbsoluteTransforms()[0];
		pos += target * speed * time_elapsed;
		transform.SetPosition(pos);
		UpdateFrustum();
	}

	void EditorCamera::MoveBackward(float time_elapsed) {
		auto& transform = *GetEntity()->GetComponent<TransformComponent>();
		glm::vec3 pos = transform.GetAbsoluteTransforms()[0];
		pos -= target * speed * time_elapsed;
		transform.SetPosition(pos);
		UpdateFrustum();
	}

	void EditorCamera::StrafeLeft(float time_elapsed) {
		auto& transform = *GetEntity()->GetComponent<TransformComponent>();
		glm::vec3 pos = transform.GetAbsoluteTransforms()[0];
		pos += -right * speed * time_elapsed;
		transform.SetPosition(pos);
		UpdateFrustum();
	}

	void EditorCamera::StrafeRight(float time_elapsed) {
		auto& transform = *GetEntity()->GetComponent<TransformComponent>();
		glm::vec3 pos = transform.GetAbsoluteTransforms()[0];
		pos += right * speed * time_elapsed;
		transform.SetPosition(pos);
		UpdateFrustum();
	}

	void EditorCamera::MoveUp(float time_elapsed) {
		auto& transform = *GetEntity()->GetComponent<TransformComponent>();
		glm::vec3 pos = transform.GetAbsoluteTransforms()[0];
		pos += speed * up * time_elapsed;
		transform.SetPosition(pos);
		UpdateFrustum();
	}

	void EditorCamera::MoveDown(float time_elapsed) {
		auto& transform = *GetEntity()->GetComponent<TransformComponent>();
		glm::vec3 pos = transform.GetAbsoluteTransforms()[0];
		pos -= speed * up * time_elapsed;
		transform.SetPosition(pos);
		UpdateFrustum();
	}
}