#ifndef CAMERACOMPONENT_H
#define CAMERACOMPONENT_H

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

		void Update();
		void MakeActive();
		glm::mat4x4 GetProjectionMatrix() const;
		// TODO: move functionality outside
		void UpdateFrustum();



		ExtraMath::Frustum view_frustum;
		float fov = 90.0f;
		float zNear = 0.1f;
		float zFar = 10000.0f;
		float exposure = 1.f;
		float aspect_ratio = 16.0 / 9.0;
	protected:
		bool is_active = false;
	};
}

#endif