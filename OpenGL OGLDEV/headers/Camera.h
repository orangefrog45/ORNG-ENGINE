#pragma once
#include <glm/glm.hpp>
#include "KeyboardState.h"
#include "TimeStep.h"

class Camera {
public:
	Camera() = default;
	Camera(int windowWidth, int windowHeight, TimeStep* timeStep);
	void SetPosition(float x, float y, float z);
	void OnMouse(const glm::vec2& newMousePos);
	void MoveForward();
	void MoveBackward();
	void StrafeLeft();
	void StrafeRight();
	void MoveUp();
	void MoveDown();
	void HandleInput(KeyboardState*);
	glm::fvec3 GetPos();
	glm::fmat4x4 GetMatrix();

private:
	int m_windowWidth;
	int m_windowHeight;
	TimeStep* timeStep;
	glm::vec2 m_oldMousePosition;
	glm::fvec3 m_pos;
	glm::fvec3 m_target;
	glm::fvec3 m_up;
	float m_speed;

};