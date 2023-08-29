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

	class PhysicsCompBase : public Component {
		friend class PhysicsSystem;
		friend class EditorLayer;
		friend class SceneSerializer;
	public:
		PhysicsCompBase(SceneEntity* p_entity) : Component(p_entity) {};

		virtual ~PhysicsCompBase() = default;
		enum GeometryType {
			BOX = 0,
			SPHERE = 1,
			TRIANGLE_MESH = 2,
		};
		enum RigidBodyType {
			STATIC = 0,
			DYNAMIC = 1,
		};

		void UpdateGeometry(GeometryType type);

	protected:
		void SendUpdateEvent();

		physx::PxShape* p_shape = nullptr;
		physx::PxMaterial* p_material = nullptr;
		physx::PxRigidActor* p_rigid_actor = nullptr;
		GeometryType geometry_type = BOX;
	};

	class PhysicsComponentDynamic : public PhysicsCompBase {
	public:
		friend class PhysicsSystem;
		PhysicsComponentDynamic(SceneEntity* p_entity) : PhysicsCompBase(p_entity) {};

		void SetVelocity(glm::vec3 v);
		glm::vec3 GetVelocity() const;
		void AddForce(glm::vec3 force);

	};

	class PhysicsComponentStatic : public PhysicsCompBase {
	public:
		friend class PhysicsSystem;
		PhysicsComponentStatic(SceneEntity* p_entity) : PhysicsCompBase(p_entity) {};

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