#include "pch/pch.h"
#include "components/PhysicsComponent.h"
#include "events/EventManager.h"
#include "physx/physx/include/characterkinematic/PxCapsuleController.h"
#include "physx/physx/include/PxRigidDynamic.h"

namespace ORNG {
	using namespace physx;

	void PhysicsComponent::SetBodyType(RigidBodyType type) {
		rigid_body_type = type;
		SendUpdateEvent();
	}

	void PhysicsComponent::UpdateGeometry(GeometryType type) {
		geometry_type = type;
		SendUpdateEvent();
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