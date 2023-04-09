#pragma once
#include <glm/glm.hpp>
#include "ExtraMath.h"

class Camera {
public:
	friend class InputHandle;
	friend class Renderer;
	Camera() = default;
	void SetPosition(float x, float y, float z);
	void OnMouse(float mouse_x, float mouse_y);
	void MoveForward(float time_elapsed);
	void MoveBackward(float time_elapsed);
	void StrafeLeft(float time_elapsed);
	void StrafeRight(float time_elapsed);
	void MoveUp(float time_elapsed);
	void MoveDown(float time_elapsed);
	void UpdateFrustum();
	glm::fvec3 GetPos() const;
	glm::fvec3 GetTarget() const { return m_target; };
	glm::fmat4x4 GetViewMatrix() const;
	glm::fmat4x4 GetProjectionMatrix() const;

private:
	ExtraMath::Frustum m_view_frustum;
	float m_fov = 90.0f;
	float m_zNear = 0.1f;
	float m_zFar = 25000.0f;
	glm::vec2 m_oldMousePosition = glm::fvec2(0.0f, 0.0f);
	glm::fvec3 m_pos = glm::fvec3(0.0f, 0.0f, 0.0f);
	glm::vec3 m_right = { 0.f, 0.f,0.f };
	glm::fvec3 m_target = glm::fvec3(0.0f, 0.0f, -1.0f);
	glm::fvec3 m_up = glm::fvec3(0.0f, 1.0f, 0.0f);
	float m_speed = 0.0001f;

};