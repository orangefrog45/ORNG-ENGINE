#include "pch/pch.h"
#include "components/PhysicsComponent.h"
#include "events/EventManager.h"
#include "physx/physx/include/characterkinematic/PxCapsuleController.h"
#include "physx/physx/include/PxRigidDynamic.h"
#include "physx/physx/include/PxRigidBody.h"

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

	void PhysicsComponent::SetVelocity(glm::vec3 v) {
	}
	glm::vec3 PhysicsComponent::GetVelocity() const {
		auto vec = ((PxRigidDynamic*)p_rigid_actor)->getLinearVelocity();
		return glm::vec3(vec.x, vec.y, vec.z);
	}

	void PhysicsComponent::SendUpdateEvent() {
		Events::ECS_Event<PhysicsComponent> phys_event;
		phys_event.affected_components.push_back(this);
		phys_event.event_type = Events::ECS_EventType::COMP_UPDATED;

		Events::EventManager::DispatchEvent(phys_event);
	}



	void CharacterControllerComponent::Move(glm::vec3 disp, float minDist, float elapsedTime) {
		mp_controller->move(PxVec3(disp.x, disp.y, disp.z), minDist, elapsedTime, 0);
	}

}