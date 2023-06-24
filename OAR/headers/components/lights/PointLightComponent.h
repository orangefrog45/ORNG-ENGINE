#pragma once
#include "components/TransformComponent.h"
#include "components/lights/BaseLight.h"

namespace ORNG {

	struct PointLightComponent : public BaseLight {

		PointLightComponent(SceneEntity* p_entity) : BaseLight(p_entity) { };
		virtual ~PointLightComponent() = default;

		float max_distance = 48.0f;
		LightAttenuation attenuation;
		TransformComponent transform;

	};

}