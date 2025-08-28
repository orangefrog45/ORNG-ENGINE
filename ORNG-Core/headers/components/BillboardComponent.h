#pragma once
#include "Component.h"

namespace ORNG {
	class Material;
	class MeshInstanceGroup;

	struct BillboardComponent final : public Component {
		friend class MeshInstancingSystem;
		explicit BillboardComponent(SceneEntity* p_entity) : Component(p_entity) {}
		BillboardComponent& operator=(const BillboardComponent&) = default;
		BillboardComponent(const BillboardComponent&) = default;
		~BillboardComponent() override = default;

		Material* p_material = nullptr;
	private:
		MeshInstanceGroup* mp_instance_group = nullptr;
	};
}
