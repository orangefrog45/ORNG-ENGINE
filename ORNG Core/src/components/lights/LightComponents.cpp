#include "pch/pch.h"
#include "components/lights/DirectionalLight.h"
#include "components/lights/SpotLightComponent.h"
#include "components/lights/PointLightComponent.h"
#include "util/ExtraMath.h"
#include "components/ComponentSystems.h"

namespace ORNG {

	void PointLightComponent::SetShadowsEnabled(bool v) {
		shadows_enabled = v;
		//mp_system->OnDepthMapUpdate();
	}

	DirectionalLight::DirectionalLight() : BaseLight(0) {
		color = glm::vec3(0.922f, 0.985f, 0.875f) * 5.f;

	}

	void SpotLightComponent::SetLightDirection(float i, float j, float k) {
		m_light_direction_vec = glm::normalize(glm::vec3(i, j, k));
	}

	SpotLightComponent::SpotLightComponent(SceneEntity* p_entity) : PointLightComponent(p_entity)
	{

		shadow_distance = 128.f;
		attenuation.constant = 1.0f;
		attenuation.linear = 0.001f;
		attenuation.exp = 0.0001f;

	};


}