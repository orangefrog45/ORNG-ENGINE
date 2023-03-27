#pragma once
#include <glm/glm.hpp>
#include <memory>
#include "TimeStep.h"

class Camera {
public:
	friend class InputHandle;
	Camera() = default;
	Camera(int windowWidth, int windowHeight);
	void SetPosition(float x, float y, float z);
	void OnMouse(float mouse_x, float mouse_y);
	void MoveForward(float time_elapsed);
	void MoveBackward(float time_elapsed);
	void StrafeLeft(float time_elapsed);
	void StrafeRight(float time_elapsed);
	void MoveUp(float time_elapsed);
	void MoveDown(float time_elapsed);
	glm::fvec3 GetPos() const;
	glm::fvec3 GetTarget() const { return m_target; };
	glm::fmat4x4 GetMatrix() const;

private:
	int m_window_width;
	int m_window_height;
	glm::vec2 m_oldMousePosition = glm::fvec2(0.0f, 0.0f);
	glm::fvec3 m_pos = glm::fvec3(0.0f, 0.0f, 0.0f);
	glm::fvec3 m_target = glm::fvec3(0.0f, 0.0f, -1.0f);
	glm::fvec3 m_up = glm::fvec3(0.0f, 1.0f, 0.0f);
	float m_speed = 0.00001f;

};