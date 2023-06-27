#pragma once
#include "Component.h"

namespace physx {
	class PxRigidDynamic;
	class PxRigidStatic;
	class PxShape;
	class PxMaterial;
	class PxScene;
}

namespace ORNG {


	struct PhysicsComponent : public Component {
		enum RigidBodyType {
			STATIC = 0,
			DYNAMIC = 1
		};

		enum GeometryType {
			BOX = 0,
			SPHERE = 1
		};

		PhysicsComponent(SceneEntity* p_entity) : Component(p_entity) {};
		~PhysicsComponent();
		void Init(physx::PxScene* p_scene, glm::vec3 half_extents, float static_friction, float dynamic_friction, float restitution, RigidBodyType type);
		void SetGeometryType(GeometryType type);


		physx::PxRigidDynamic* p_rigid_dynamic = nullptr;
		physx::PxRigidStatic* p_rigid_static = nullptr;
		physx::PxShape* p_shape = nullptr;
		physx::PxMaterial* p_material = nullptr;
		physx::PxScene* p_scene = nullptr;

		RigidBodyType rigid_body_type = STATIC;
	};

}