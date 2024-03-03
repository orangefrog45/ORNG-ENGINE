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
	class PxFixedJoint;
	class PxActor;
	class PxSphericalJoint;
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



	class CharacterControllerComponent : public Component {
	public:
		friend class PhysicsSystem;
		explicit CharacterControllerComponent(SceneEntity* p_entity) : Component(p_entity) {};

		// Movement will update transform at the end of each frame and can be overwritten by calling functions like LookAt
		// To avoid overwriting, ensure this is called AFTER any transform updates to the entity it is attached to
		void Move(glm::vec3 disp, float minDist, float elapsedTime);
		bool moved_during_frame = false;
		physx::PxController* mp_controller = nullptr;
	private:
	};

	enum JointEventType {
		CONNECT,
		BREAK
	};

	class FixedJointComponent : public Component {
	public:
		friend class PhysicsSystem;
		friend class EditorLayer;
		FixedJointComponent(SceneEntity* p_entity) : Component(p_entity) {};
		/*void Connect(PhysicsComponent* t_a0, PhysicsComponent* t_a1) {
		Events::ECS_Event<FixedJointComponent> joint_event{ Events::ECS_EventType::COMP_UPDATED, this, JointEventType::CONNECT };
		joint_event.data_payload = std::make_pair(t_a0, t_a1);
		Events::EventManager::DispatchEvent(joint_event);
		}*/
		void Break();

	private:
		physx::PxFixedJoint* mp_joint = nullptr;
	};

}

#endif