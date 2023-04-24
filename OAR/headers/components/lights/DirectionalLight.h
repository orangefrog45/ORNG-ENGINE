#pragma once
#include "components/lights/BaseLight.h"
#include "WorldTransform.h"

class DirectionalLight : public BaseLight {
public:
	friend class EditorLayer;
	DirectionalLight();
	auto GetLightDirection() const { return m_light_direction; };
	void SetLightDirection(const glm::vec3& dir);
	const auto& GetTransformMatrix() const { return m_light_transform_matrix; }
private:
	glm::mat4 m_light_transform_matrix;
	glm::vec3 m_light_direction = glm::vec3(0.0f, 0.0f, -1.f);
};

