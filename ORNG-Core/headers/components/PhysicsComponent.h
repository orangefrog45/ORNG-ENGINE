#ifndef PHYSICSCOMPONENT_H
#define PHYSICSCOMPONENT_H

#include "Component.h"
#include "events/EventManager.h"

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




	class PhysicsComponent : public Component {
		friend class PhysicsSystem;
		friend class EditorLayer;
		friend class SceneSerializer;
		friend class FixedJointComponent;
	public:
		PhysicsComponent(SceneEntity* p_entity) : Component(p_entity) {};

		void SetVelocity(glm::vec3 v);
		glm::vec3 GetVelocity() const;

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

	protected:
		void SendUpdateEvent();

		physx::PxShape* p_shape = nullptr;
		physx::PxMaterial* p_material = nullptr;
		physx::PxRigidActor* p_rigid_actor = nullptr;
		GeometryType m_geometry_type = BOX;
		RigidBodyType m_body_type = STATIC;
	};



	class CharacterControllerComponent : public Component {
	public:
		friend class PhysicsSystem;
		explicit CharacterControllerComponent(SceneEntity* p_entity) : Component(p_entity) {};
		void Move(glm::vec3 disp, float minDist, float elapsedTime);
	private:
		physx::PxController* mp_controller = nullptr;
	};

	enum JointEventType {
		CONNECT,
		BREAK
	};

	struct FixedJointComponent : public Component {
		friend class PhysicsSystem;
		friend class EditorLayer;
		FixedJointComponent(SceneEntity* p_entity) : Component(p_entity) {};
		void Connect(PhysicsComponent* t_a0, PhysicsComponent* t_a1) {
					Events::ECS_Event<FixedJointComponent> joint_event;
		joint_event.affected_components[0] = this;
		joint_event.event_type = Events::ECS_EventType::COMP_UPDATED;
		joint_event.sub_event_type = static_cast<uint8_t>(JointEventType::CONNECT);
		joint_event.data_payload = std::make_pair(t_a0, t_a1);
		Events::EventManager::DispatchEvent(joint_event);
		}
		void Break();

	private:
		physx::PxFixedJoint* mp_joint = nullptr;
	};

}

#endif