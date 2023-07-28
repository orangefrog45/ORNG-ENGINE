#pragma once
#include "util/ExtraMath.h"
#include "Component.h"

namespace ORNG {
	class CameraSystem;
	class TransformComponent;

	struct CameraComponent : public Component {
		friend class SceneRenderer;
		friend class Scene;
		friend class CameraSystem;
		CameraComponent(SceneEntity* p_entity) : Component(p_entity) {};

		virtual void Update();
		void MakeActive();
		glm::mat4x4 GetViewMatrix();
		glm::mat4x4 GetProjectionMatrix() const;
		void UpdateFrustum();



		ExtraMath::Frustum view_frustum;
		bool mouse_locked = false;
		float fov = 90.0f;
		float zNear = 0.1f;
		float zFar = 10000.0f;
		float exposure = 1.f;
		glm::vec3 right = { 1.f, 0.f,0.f };
		glm::vec3 target = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		float speed = 0.01f;
	protected:
		bool is_active = false;
	};
}