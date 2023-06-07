#pragma once
#include "components/Component.h"
#include "components/WorldTransform.h"

namespace ORNG {
	struct FogVolumeComponent : public Component {
		//glm::vec3 color;
		WorldTransform transform;
	};
}