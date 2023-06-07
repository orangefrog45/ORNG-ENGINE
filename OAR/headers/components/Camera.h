#pragma once
#include "util/ExtraMath.h"

namespace ORNG {

	class Camera {
	public:
		friend class SceneRenderer;
		Camera() = default;
		void SetPosition(float x, float y, float z);
		void MoveForward(float time_elapsed);
		void MoveBackward(float time_elapsed);
		void StrafeLeft(float time_elapsed);
		void StrafeRight(float time_elapsed);
		void MoveUp(float time_elapsed);
		void MoveDown(float time_elapsed);
		void Update();
		float GetFovDegrees() const { return m_fov; }
		glm::vec3 GetPos() const { return m_pos; };
		glm::vec3 GetTarget() const { return m_target; };
		glm::mat4x4 GetViewMatrix() const;
		glm::mat4x4 GetProjectionMatrix() const;

	private:
		ExtraMath::Frustum m_view_frustum;
		void UpdateFrustum();
		bool m_mouse_locked = false;
		float m_fov = 90.0f;
		float m_zNear = 0.1f;
		float m_zFar = 10000.0f;
		glm::vec2 m_oldMousePosition = glm::vec2(0.0f, 0.0f);
		glm::vec3 m_pos = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 m_right = { 1.f, 0.f,0.f };
		glm::vec3 m_target = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 m_up = glm::vec3(0.0f, 1.0f, 0.0f);
		float m_speed = 0.00001f;

	};
}