#pragma once
#include "components/WorldTransform.h"
#include "components/lights/BaseLight.h"

namespace ORNG {

	struct PointLightComponent : public BaseLight {
		PointLightComponent(unsigned int entity_id) : BaseLight(entity_id) {};
		virtual ~PointLightComponent() = default;

		float max_distance = 48.0f;
		LightAttenuation attenuation;
		WorldTransform transform;
	};

}