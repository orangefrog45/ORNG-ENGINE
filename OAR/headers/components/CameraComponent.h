#pragma once
#include "util/ExtraMath.h"
#include "Component.h"

namespace ORNG {

	struct CameraComponent : public Component {
		friend class SceneRenderer;
		friend class Scene;
		CameraComponent(SceneEntity* p_entity) : Component(p_entity) {};
		void SetPosition(float x, float y, float z);
		void MoveForward(float time_elapsed);
		void MoveBackward(float time_elapsed);
		void StrafeLeft(float time_elapsed);
		void StrafeRight(float time_elapsed);
		void MoveUp(float time_elapsed);
		void MoveDown(float time_elapsed);
		void Update();
		glm::mat4x4 GetViewMatrix() const;
		glm::mat4x4 GetProjectionMatrix() const;
		void UpdateFrustum();

		ExtraMath::Frustum view_frustum;
		bool is_active = false;
		bool mouse_locked = false;
		float fov = 90.0f;
		float zNear = 0.1f;
		float zFar = 10000.0f;
		glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 right = { 1.f, 0.f,0.f };
		glm::vec3 target = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		float speed = 0.01f;

	};
}