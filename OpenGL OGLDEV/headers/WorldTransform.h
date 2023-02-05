#pragma once
#include <glm/glm.hpp>
class WorldTransform
{
public:
	WorldTransform();

	void SetScale(float scaleX, float scaleY, float scaleZ);
	void SetRotation(float x, float y, float z);
	void SetPosition(float x, float y, float z);

	void Rotate(float x, float y, float z);
	glm::fmat4x4 GetMatrix();


private:
	glm::fvec3 m_scale = glm::fvec3(1.0f, 1.0f, 1.0f);
	glm::fvec3 m_rotation = glm::fvec3(0.0f, 0.0f, 0.0f);
	glm::fvec3 m_pos = glm::fvec3(0.0f, 0.0f, 0.0f);

};

