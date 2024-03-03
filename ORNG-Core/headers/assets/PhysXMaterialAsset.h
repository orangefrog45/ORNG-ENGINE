#include "Asset.h"
#include "physx/PxMaterial.h"

namespace ORNG {
	struct PhysXMaterialAsset : public Asset {
		PhysXMaterialAsset(const std::string& t_filepath) : Asset(t_filepath) {};

		template<typename S>
		void serialize(S& s) {
			s.object(uuid);
			s.container1b(name, ORNG_MAX_FILEPATH_SIZE);
			s.value4b(p_material->getStaticFriction());
			s.value4b(p_material->getDynamicFriction());
			s.value4b(p_material->getRestitution());
		}

		std::string name = "PhysX material";

		physx::PxMaterial* p_material = nullptr;
	};
}