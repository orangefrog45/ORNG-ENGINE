#include "pch/pch.h"
#include "components/EditorCamera.h"
#include "core/Window.h"
#include "core/FrameTiming.h"

namespace ORNG {

	void EditorCamera::Update() {
		if (Window::IsKeyDown('W')) {
			MoveForward(FrameTiming::GetTimeStep());
		}

		if (Window::IsKeyDown('A'))
			StrafeLeft(FrameTiming::GetTimeStep());

		if (Window::IsKeyDown('S'))
			MoveBackward(FrameTiming::GetTimeStep());

		if (Window::IsKeyDown('D'))
			StrafeRight(FrameTiming::GetTimeStep());

		if (Window::IsKeyDown('Q'))
			MoveDown(FrameTiming::GetTimeStep());

		if (Window::IsKeyDown('E'))
			MoveUp(FrameTiming::GetTimeStep());

		if (Window::IsKeyDown('R'))
			mouse_locked = true;

		if (Window::IsKeyDown('T'))
			mouse_locked = false;



		if (!mouse_locked) {
			glm::vec2 mouse_coords = Window::GetMousePos();
			float rotation_speed = 0.005f;
			glm::vec2 mouseDelta = -glm::vec2(mouse_coords.x - Window::GetWidth() / 2, mouse_coords.y - Window::GetHeight() / 2);

			target = glm::rotate(mouseDelta.x * rotation_speed, up) * glm::vec4(target, 0);
			UpdateFrustum();

			glm::fvec3 targetNew = glm::rotate(mouseDelta.y * rotation_speed, glm::cross(target, up)) * glm::vec4(target, 0);
			//constraint to stop lookAt flipping from y axis alignment
			if (targetNew.y <= 0.9996f && targetNew.y >= -0.996f) {
				target = targetNew;
			}
			target = glm::normalize(target);

			Window::SetCursorPos(Window::GetWidth() / 2, Window::GetHeight() / 2);
		}

	}
}