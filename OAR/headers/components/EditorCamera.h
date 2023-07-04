#pragma once
#include "components/CameraComponent.h"
#include "components/TransformComponent.h"

namespace ORNG {

	class CameraSystem;
	struct EditorCamera : public CameraComponent {
		friend class EditorLayer;
		explicit EditorCamera(CameraSystem* p_system) : CameraComponent(nullptr, p_system, new TransformComponent) {};
		void Update();
		void MoveForward(float time_elapsed);
		void MoveBackward(float time_elapsed);
		void StrafeLeft(float time_elapsed);
		void StrafeRight(float time_elapsed);
		void MoveUp(float time_elapsed);
		void MoveDown(float time_elapsed);

	};
}