#pragma once
#include "components/TransformComponent.h"
#include "components/lights/BaseLight.h"

namespace ORNG {

	class PointlightSystem;

	struct PointLightComponent : public BaseLight {
		PointLightComponent(SceneEntity* p_entity) : BaseLight(p_entity) { };

		void SetShadowsEnabled(bool v);
		float shadow_distance = 48.0f;
		LightAttenuation attenuation;

	};

}