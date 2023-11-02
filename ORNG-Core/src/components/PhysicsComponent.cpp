#include "pch/pch.h"
#include "components/PhysicsComponent.h"
#include "events/EventManager.h"
#include <characterkinematic/PxCapsuleController.h>
#include <PxPhysicsAPI.h>
#include "physics/Physics.h"

namespace ORNG {
	using namespace physx;

	void PhysicsComponent::UpdateGeometry(GeometryType type) {
		m_geometry_type = type;
		SendUpdateEvent();
	}

	void PhysicsComponent::SetBodyType(RigidBodyType type) {
		m_body_type = type;
		SendUpdateEvent();
	}

	void PhysicsComponent::AddForce(glm::vec3 force) {
		if (m_body_type == DYNAMIC)
			((PxRigidDynamic*)p_rigid_actor)->addForce(PxVec3(force.x, force.y, force.z));
	}

	void PhysicsComponent::ToggleGravity(bool on) {
		if (m_body_type == DYNAMIC) {
			((PxRigidDynamic*)p_rigid_actor)->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, !on);
		}
	}

	void PhysicsComponent::SetMass(float mass) {
		if (m_body_type == DYNAMIC && mass >= 0.f)
			((PxRigidDynamic*)p_rigid_actor)->setMass(mass);
	}


	void PhysicsComponent::SetVelocity(glm::vec3 v) {
		if (m_body_type == DYNAMIC)
			((PxRigidDynamic*)p_rigid_actor)->setLinearVelocity(PxVec3(v.x, v.y, v.z));
	}

	void PhysicsComponent::SetAngularVelocity(glm::vec3 v) {
		if (m_body_type == DYNAMIC)
			((PxRigidDynamic*)p_rigid_actor)->setAngularVelocity(PxVec3(v.x, v.y, v.z));
	}

	glm::vec3 PhysicsComponent::GetAngularVelocity() const {
		auto vec = ((PxRigidDynamic*)p_rigid_actor)->getAngularVelocity();
		return glm::vec3(vec.x, vec.y, vec.z);
	}

	glm::vec3 PhysicsComponent::GetVelocity() const {
		auto vec = ((PxRigidDynamic*)p_rigid_actor)->getLinearVelocity();
		return glm::vec3(vec.x, vec.y, vec.z);
	}

	void PhysicsComponent::SendUpdateEvent() {
		Events::ECS_Event<PhysicsComponent> phys_event;
		phys_event.affected_components[0] = this;
		phys_event.event_type = Events::ECS_EventType::COMP_UPDATED;

		Events::EventManager::DispatchEvent(phys_event);
	}



	void CharacterControllerComponent::Move(glm::vec3 disp, float minDist, float elapsedTime) {
		mp_controller->move(PxVec3(disp.x, disp.y, disp.z), minDist, elapsedTime, 0);
	}

	


}