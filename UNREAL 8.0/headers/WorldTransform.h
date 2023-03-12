#pragma once
#include <glm/glm.hpp>
class WorldTransform2D {
public:

	void SetScale(float x, float y);
	void SetRotation(float rot);
	void SetPosition(float x, float y);

	glm::fmat3 GetMatrix() const;
	glm::fvec2 GetPosition() const;

private:
	glm::fvec2 m_scale = glm::fvec2(1.0f, 1.0f);
	float m_rotation = 0.0f;
	glm::fvec2 m_pos = glm::fvec2(0.0f, 0.0f);
};

class WorldTransform
{
public:
	WorldTransform() = default;

	void SetScale(float scaleX, float scaleY, float scaleZ);
	void SetRotation(float x, float y, float z);
	void SetPosition(float x, float y, float z);

	void Rotate(float x, float y, float z);
	glm::fmat4x4 GetMatrix() const;
	glm::fvec3 GetPosition() const;


private:
	glm::fvec3 m_scale = glm::fvec3(1.0f, 1.0f, 1.0f);
	glm::fvec3 m_rotation = glm::fvec3(0.0f, 0.0f, 0.0f);
	glm::fvec3 m_pos = glm::fvec3(0.0f, 0.0f, 0.0f);

};

