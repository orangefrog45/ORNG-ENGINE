#pragma once
#include "components/Component.h"

namespace ORNG {

	struct BaseLight : public Component {
		virtual ~BaseLight() = default;
		BaseLight(SceneEntity* p_entity) : Component(p_entity) {};

		glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
		bool shadows_enabled = false;
	};

	struct LightAttenuation {
		float constant = 1.0f;
		float linear = 0.05f;
		float exp = 0.01f;
	};

}
