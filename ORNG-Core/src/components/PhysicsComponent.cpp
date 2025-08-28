#include "pch/pch.h"
#include "components/PhysicsComponent.h"
#include "events/EventManager.h"
#include "scene/SceneEntity.h"

namespace ORNG {
	void PhysicsComponent::UpdateGeometry(GeometryType type) {
		m_geometry_type = type;
		SendUpdateEvent();
	}

	void PhysicsComponent::SetBodyType(RigidBodyType type) {
		m_body_type = type;
		SendUpdateEvent();
	}

	void PhysicsComponent::SetTrigger(bool is_trigger) {
		m_is_trigger = is_trigger;
		SendUpdateEvent(); // Shape needs recreating
	}

	void PhysicsComponent::SendUpdateEvent() {
		Events::ECS_Event<PhysicsComponent> phys_event{ Events::ECS_EventType::COMP_UPDATED, this };
		Events::EventManager::DispatchEvent(phys_event);
	}
}
