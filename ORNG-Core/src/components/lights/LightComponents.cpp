#include "pch/pch.h"
#include "components/Lights.h"
#include "util/ExtraMath.h"
#include "components/ComponentSystems.h"

namespace ORNG {
	void PointLightComponent::SetShadowsEnabled(bool v) {
		shadows_enabled = v;
		//mp_system->OnDepthMapUpdate();
	}

	DirectionalLight::DirectionalLight() : BaseLight(nullptr) {
		colour = glm::vec3(0.922f, 0.985f, 0.875f) * 5.f;
		shadows_enabled = true;
	}


	SpotLightComponent::SpotLightComponent(SceneEntity* p_entity) : PointLightComponent(p_entity)
	{
		shadow_distance = 128.f;
		attenuation.constant = 1.0f;
		attenuation.linear = 0.001f;
		attenuation.exp = 0.0001f;
	};
}
