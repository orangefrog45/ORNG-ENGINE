#pragma once
#include "components/Component.h"

namespace ORNG {
	struct DecalComponent final : public Component {
		explicit DecalComponent(SceneEntity* p_ent) : Component(p_ent) {}
		DecalComponent& operator=(const DecalComponent&) = default;
		DecalComponent(const DecalComponent&) = default;
		~DecalComponent() override = default;

		// If p_material is nullptr, this decal will not be rendered
		class Material* p_material = nullptr;
	};
}
