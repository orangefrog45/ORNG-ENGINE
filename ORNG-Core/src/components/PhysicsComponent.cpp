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

	void PhysicsComponent::AddForce(glm::vec3 force) {
	}

	void PhysicsComponent::ToggleGravity(bool on) {
	}

	void PhysicsComponent::SetMass(float mass) {
	}


	void PhysicsComponent::SetVelocity(glm::vec3 v) {
	}

	void PhysicsComponent::SetAngularVelocity(glm::vec3 v) {
	}

	glm::vec3 PhysicsComponent::GetAngularVelocity() const {
		return { 0, 0, 0 };
	}

	glm::vec3 PhysicsComponent::GetVelocity() const {
		return { 0, 0, 0 };
	}

	void PhysicsComponent::SendUpdateEvent() {
		Events::ECS_Event<PhysicsComponent> phys_event{ Events::ECS_EventType::COMP_UPDATED, this };
		Events::EventManager::DispatchEvent(phys_event);
	}

	void CharacterControllerComponent::Move(glm::vec3 disp, float minDist, float elapsedTime) {
		moved_during_frame = true;
	}
}