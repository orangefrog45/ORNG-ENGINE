#include "pch/pch.h"

#include "components/lights/SpotLightComponent.h"
#include "scene/SceneEntity.h"

namespace ORNG {

	void SpotLightComponent::SetLightDirection(float i, float j, float k) {
		m_light_direction_vec = glm::normalize(glm::vec3(i, j, k));

		UpdateLightTransform();
	}

	SpotLightComponent::SpotLightComponent(SceneEntity* p_entity, TransformComponent* t_transform) : PointLightComponent(p_entity, t_transform)
	{
		max_distance = 128.f;
		attenuation.constant = 1.0f;
		attenuation.linear = 0.001f;
		attenuation.exp = 0.0001f;

		UpdateLightTransform();
	};

	void SpotLightComponent::UpdateLightTransform() {
		float z_near = 0.1f;
		float z_far = max_distance;
		auto light_perspective = glm::perspective(glm::degrees(acosf(aperture)), 1.0f, z_near, z_far);
		glm::vec3 pos = p_transform->GetAbsoluteTransforms()[0];
		auto spot_light_view = glm::lookAt(pos, pos + m_light_direction_vec, glm::vec3(0.0f, 1.0f, 0.0f));


		m_light_space_transform = glm::fmat4(light_perspective * spot_light_view);
	}

}