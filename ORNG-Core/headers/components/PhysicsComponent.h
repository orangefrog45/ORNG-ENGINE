#ifndef PHYSICSCOMPONENT_H
#define PHYSICSCOMPONENT_H

#include "Component.h"
#include "physx/extensions/PxD6Joint.h"
#include "scene/EntityNodeRef.h"

namespace physx {
	class PxRigidDynamic;
	class PxRigidStatic;
	class PxRigidActor;
	class PxShape;
	class PxMaterial;
	class PxScene;
	class PxController;
	class PxActor;
	class PxD6Joint;
}

namespace ORNG {
	class PhysicsSystem;
	class TransformComponent;
	struct PhysXMaterialAsset;



	class PhysicsComponent : public Component {
		friend class PhysicsSystem;
		friend class EditorLayer;
		friend class SceneSerializer;
		friend class FixedJointComponent;
	public:
		PhysicsComponent(SceneEntity* p_entity) : Component(p_entity) {};

		void SetVelocity(glm::vec3 v);
		void SetAngularVelocity(glm::vec3 v);
		glm::vec3 GetVelocity() const;
		glm::vec3 GetAngularVelocity() const;

		void SetMaterial(PhysXMaterialAsset& material);

		void AddForce(glm::vec3 force);
		void ToggleGravity(bool on);
		void SetMass(float mass);

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
		void SetBodyType(RigidBodyType type);

		void SetTrigger(bool is_trigger);
		bool IsTrigger() { return m_is_trigger; }

		PhysXMaterialAsset* GetMaterial() {
			return p_material;
		}


	private:
		void SendUpdateEvent();

		bool m_is_trigger = false;
		physx::PxShape* p_shape = nullptr;
		PhysXMaterialAsset* p_material = nullptr;
		physx::PxRigidActor* p_rigid_actor = nullptr;
		GeometryType m_geometry_type = BOX;
		RigidBodyType m_body_type = STATIC;
	};



	struct CharacterControllerComponent : public Component {
	public:
		friend class PhysicsSystem;
		explicit CharacterControllerComponent(SceneEntity* p_entity) : Component(p_entity) {};

		// Movement will update transform at the end of each frame and can be overwritten by calling functions like LookAt afterwards
		// To avoid overwriting, ensure this is called AFTER any transform updates to the entity it is attached to
		void Move(glm::vec3 disp, float minDist, float elapsedTime);
		bool moved_during_frame = false;

		physx::PxController* p_controller = nullptr;
	};

	enum JointEventType {
		CONNECT,
		BREAK
	};

	struct JointComponent : public Component {

		struct ConnectionData {
			ConnectionData(PhysicsComponent* _0, PhysicsComponent* _1, bool _use_comp_poses) : first(_0), second(_1), use_component_poses(_use_comp_poses) {};

			PhysicsComponent* first = nullptr;
			PhysicsComponent* second = nullptr;

			// If false, physics system will place the joint in the middle of the two components instead of using the poses in this->poses
			bool use_component_poses = false;
		};

		void Connect(PhysicsComponent* t_a0, PhysicsComponent* t_a1, bool use_comp_poses = false);
		void Break();

		void SetMotion(physx::PxD6Axis::Enum axis, physx::PxD6Motion::Enum _motion) {
			if (p_joint)
				p_joint->setMotion(axis, _motion);

			motion[axis] = _motion;
		}

		void SetLocalPose(unsigned actor_idx, glm::vec3 p) {
			if (p_joint)
				p_joint->setLocalPose((physx::PxJointActorIndex::Enum)actor_idx, physx::PxTransform{ physx::PxVec3(p.x, p.y, p.z) });

			poses[actor_idx] = p;
		}

		EntityNodeRef attachment_ref0{ GetEntity() };
		EntityNodeRef attachment_ref1{ GetEntity() };

		std::array<glm::vec3, 2> poses;

		std::unordered_map<physx::PxD6Axis::Enum, physx::PxD6Motion::Enum> motion;
		physx::PxD6Joint* p_joint = nullptr;
	};

}

#endif