#pragma once
#include "components/TransformComponent.h"
#include "components/lights/BaseLight.h"

namespace ORNG {

	struct PointLightComponent : public BaseLight {

		PointLightComponent(SceneEntity* p_entity, const TransformComponent* t_transform) : BaseLight(p_entity), p_transform(t_transform) { };
		virtual ~PointLightComponent() = default;

		float max_distance = 48.0f;
		LightAttenuation attenuation;
		const TransformComponent* p_transform = nullptr;

	};

}