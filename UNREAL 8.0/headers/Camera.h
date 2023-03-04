#pragma once
#include <glm/glm.hpp>
#include "InputHandle.h"
#include "TimeStep.h"

class Camera {
public:
	Camera() = default;
	Camera(int windowWidth, int windowHeight, const std::shared_ptr<InputHandle> keyboard);
	void SetPosition(float x, float y, float z);
	void OnMouse(float mouse_x, float mouse_y);
	void MoveForward();
	void MoveBackward();
	void StrafeLeft();
	void StrafeRight();
	void MoveUp();
	void MoveDown();
	void HandleInput();
	glm::fvec3 GetPos() const;
	glm::fvec3 GetTarget() const { return m_target; };
	glm::fmat4x4 GetMatrix() const;

private:
	int m_windowWidth;
	int m_windowHeight;
	TimeStep time_step;
	std::shared_ptr<InputHandle> input_handle;
	glm::vec2 m_oldMousePosition = glm::fvec2(0.0f, 0.0f);
	glm::fvec3 m_pos = glm::fvec3(0.0f, 0.0f, 0.0f);
	glm::fvec3 m_target = glm::fvec3(0.0f, 0.0f, 1.0f);
	glm::fvec3 m_up = glm::fvec3(0.0f, 1.0f, 0.0f);
	float m_speed = 3.0f;

};