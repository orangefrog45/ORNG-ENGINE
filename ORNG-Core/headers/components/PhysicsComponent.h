#ifndef PHYSICSCOMPONENT_H
#define PHYSICSCOMPONENT_H

#include "Component.h"

namespace physx {
	class PxRigidDynamic;
	class PxRigidStatic;
	class PxRigidActor;
	class PxShape;
	class PxMaterial;
	class PxScene;
	class PxController;
}

namespace ORNG {
	class PhysicsSystem;
	class TransformComponent;

	struct PhysicsComponent : public Component {
		friend class PhysicsSystem;
		friend class EditorLayer;
		friend class SceneSerializer;
		explicit PhysicsComponent(SceneEntity* p_entity) : Component(p_entity) {};
		enum RigidBodyType {
			STATIC = 0,
			DYNAMIC = 1
		};

		enum GeometryType {
			BOX = 0,
			SPHERE = 1,
			TRIANGLE_MESH = 2,
		};

		void SetBodyType(RigidBodyType type);
		// Used for changing scale/type of geometry, e.g sphere collider to box collider
		void UpdateGeometry(GeometryType type);


	private:
		void SendUpdateEvent();

		physx::PxRigidActor* p_rigid_actor = nullptr;
		physx::PxShape* p_shape = nullptr;
		physx::PxMaterial* p_material = nullptr;

		RigidBodyType rigid_body_type = STATIC;
		GeometryType geometry_type = BOX;
	};

	class CharacterControllerComponent : public Component {
	public:
		friend class PhysicsSystem;
		explicit CharacterControllerComponent(SceneEntity* p_entity) : Component(p_entity) {};
		void Move(glm::vec3 disp, float minDist, float elapsedTime);
	private:
		physx::PxController* mp_controller = nullptr;
	};

}

#endif