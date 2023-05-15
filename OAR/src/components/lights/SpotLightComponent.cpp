#include "pch/pch.h"

#include "components/lights/SpotLightComponent.h"

namespace ORNG {

	void SpotLightComponent::SetLightDirection(float i, float j, float k) {
		m_light_direction_vec = glm::normalize(glm::vec3(i, j, k));

		if (shadows_enabled) UpdateLightTransform();
	}

	SpotLightComponent::SpotLightComponent(unsigned int entity_id) : PointLightComponent(entity_id)
	{
		max_distance = 128.f;
		attenuation.constant = 1.0f;
		attenuation.linear = 0.001f;
		attenuation.exp = 0.0001f;

		if (shadows_enabled) UpdateLightTransform();
	};

	void SpotLightComponent::UpdateLightTransform() {
		float z_near = 0.1f;
		float z_far = max_distance;
		auto light_perspective = glm::perspective(glm::degrees(acosf(aperture)) + 3.0f, 1.0f, z_near, z_far);
		auto spot_light_view = glm::lookAt(transform.GetPosition(), transform.GetPosition() + m_light_direction_vec, glm::vec3(0.0f, 1.0f, 0.0f));


		m_light_space_transform = glm::fmat4(light_perspective * spot_light_view);
	}

	void SpotLightComponent::SetPosition(const float x, const float y, const float z) {
		transform.SetPosition(x, y, z);

		if (shadows_enabled) UpdateLightTransform();
	};
}