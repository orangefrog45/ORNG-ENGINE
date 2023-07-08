#pragma once
#include "Component.h"

namespace physx {
	class PxRigidDynamic;
	class PxRigidStatic;
	class PxRigidActor;
	class PxShape;
	class PxMaterial;
	class PxScene;
}

namespace ORNG {
	class PhysicsSystem;
	class TransformComponent;

	struct PhysicsComponent : public Component {
		friend class PhysicsSystem;
		friend class EditorLayer;
		friend class SceneSerializer;
		enum RigidBodyType {
			STATIC = 0,
			DYNAMIC = 1
		};

		enum GeometryType {
			BOX = 0,
			SPHERE = 1,
			TRIANGLE_MESH = 2,
		};

		PhysicsComponent(SceneEntity* p_entity, TransformComponent* p_transform, PhysicsSystem* p_system) : Component(p_entity), mp_transform(p_transform), mp_system(p_system) {};
		~PhysicsComponent();
		void Init(physx::PxScene* p_scene, glm::vec3 half_extents, RigidBodyType type, physx::PxMaterial* t_p_material);
		void SetBodyType(RigidBodyType type);

		// Used for changing scale/type of geometry, e.g sphere collider to box collider
		void UpdateGeometry(GeometryType type);

		glm::vec3 GetPos() const { return old_pos; };
		glm::vec3 GetOrientation() const { return old_orientation; };

		physx::PxRigidActor* p_rigid_actor = nullptr;
		glm::vec3 old_pos = { 0, 0, 0 };
		glm::vec3 old_orientation = { 0, 0, 0 }; // degrees
	private:

		physx::PxShape* p_shape = nullptr;
		physx::PxMaterial* p_material = nullptr;
		TransformComponent* mp_transform = nullptr;
		physx::PxScene* p_scene = nullptr;
		PhysicsSystem* mp_system = nullptr;

		RigidBodyType rigid_body_type = STATIC;
		GeometryType geometry_type = BOX;
	};

}