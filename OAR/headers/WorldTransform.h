#pragma once
class WorldTransform2D {
public:

	void SetScale(float x, float y);
	void SetRotation(float rot);
	void SetPosition(float x, float y);

	glm::mat3 GetMatrix() const;
	glm::vec2 GetPosition() const;

private:
	glm::vec2 m_scale = glm::vec2(1.0f, 1.0f);
	float m_rotation = 0.0f;
	glm::vec2 m_pos = glm::vec2(0.0f, 0.0f);
};

class WorldTransform
{
public:
	WorldTransform() = default;

	void SetScale(float scaleX, float scaleY, float scaleZ);
	void SetRotation(float x, float y, float z);
	void SetPosition(float x, float y, float z);

	void SetPosition(const glm::vec3 pos) { m_pos = pos; };
	void SetScale(const glm::vec3 scale) { m_scale = scale; };
	void SetRotation(const glm::vec3 rot) { m_rotation = rot; };

	void Rotate(float x, float y, float z);
	glm::mat4x4 GetMatrix() const;
	glm::vec3 GetPosition() const;
	glm::vec3 GetScale() const { return m_scale; };
	glm::vec3 GetRotation() const { return m_rotation; };


private:
	glm::vec3 m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 m_rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 m_pos = glm::vec3(0.0f, 0.0f, 0.0f);

};

