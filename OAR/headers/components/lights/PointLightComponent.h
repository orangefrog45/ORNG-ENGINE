#pragma once
#include "WorldTransform.h"
#include "components/lights/BaseLight.h"


struct PointLightComponent : public BaseLight {
	PointLightComponent(unsigned int entity_id) : BaseLight(entity_id) {};
	virtual ~PointLightComponent() = default;

	float max_distance = 48.0f;
	LightAttenuation attenuation;
	WorldTransform transform;
};