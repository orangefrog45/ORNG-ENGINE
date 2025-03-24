#pragma once
#include "components/Component.h"

namespace ORNG {
	struct DecalComponent : public Component {
		DecalComponent(SceneEntity* p_ent) : Component(p_ent) {};

		// If p_material is nullptr, this decal will not be rendered
		class Material* p_material = nullptr;
	};
}