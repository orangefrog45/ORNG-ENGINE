#include "Component.h"

namespace ORNG {
	class Material;
	class MeshInstanceGroup;

	struct BillboardComponent : public Component {
		friend class MeshInstancingSystem;
		BillboardComponent(SceneEntity* p_entity) : Component(p_entity) {};
		Material* p_material = nullptr;
	private:
		MeshInstanceGroup* mp_instance_group = nullptr;
	};
}