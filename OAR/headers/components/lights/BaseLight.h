#pragma once
#include "components/Component.h"

namespace ORNG {

	struct BaseLight : public Component {
		virtual ~BaseLight() = default;
		BaseLight(SceneEntity* p_entity) : Component(p_entity) {};

		bool shadows_enabled = true;
		glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
		float ambient_intensity = 0.1f;
		float diffuse_intensity = 1.0f;
	};

	struct LightAttenuation {
		float constant = 1.0f;
		float linear = 0.05f;
		float exp = 0.01f;
	};

}
