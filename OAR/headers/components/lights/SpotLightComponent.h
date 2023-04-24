#pragma once
#include "components/lights/PointLightComponent.h"

class SpotLightComponent : public PointLightComponent {
public:
	friend class EditorLayer;
	SpotLightComponent(unsigned int entity_id);
	void SetLightDirection(float i, float j, float k);
	void SetAperture(float angle) { aperture = cosf(glm::radians(angle)); }
	void SetPosition(const float x, const float y, const float z);

	auto GetLightDirection() const { return m_light_direction_vec; }
	auto GetAperture() const { return aperture; }
	const glm::mat4& GetLightSpaceTransformMatrix() const { return m_light_space_transform; }
private:
	void UpdateLightTransform();
	glm::mat4 m_light_space_transform;
	glm::vec3 m_light_direction_vec = glm::vec3(1, 0, 0);
	float aperture = 0.9396f;
};