#pragma once
#include <glm/glm.hpp>
#include "KeyboardState.h"
#include "TimeStep.h"

class Camera {
public:
	Camera() = default;
	Camera(int windowWidth, int windowHeight, const TimeStep* time_step);
	void SetPosition(float x, float y, float z);
	void OnMouse(const glm::vec2& newMousePos);
	void MoveForward();
	void MoveBackward();
	void StrafeLeft();
	void StrafeRight();
	void MoveUp();
	void MoveDown();
	void HandleInput(const KeyboardState& keyboard_state);
	glm::fvec3 GetPos() const;
	glm::fmat4x4 GetMatrix() const;

private:
	int m_windowWidth;
	int m_windowHeight;
	const TimeStep* time_step;
	glm::vec2 m_oldMousePosition = glm::fvec2(0.0f, 0.0f);
	glm::fvec3 m_pos = glm::fvec3(0.0f, 0.0f, 0.0f);
	glm::fvec3 m_target = glm::fvec3(0.0f, 0.0f, 1.0f);
	glm::fvec3 m_up = glm::fvec3(0.0f, 1.0f, 0.0f);
	float m_speed = 0.003f;

};