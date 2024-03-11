#include "pch/pch.h"
#include "components/PhysicsComponent.h"
#include "events/EventManager.h"
#include "physics/Physics.h"
#include "assets/PhysXMaterialAsset.h"

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

	void PhysicsComponent::SetTrigger(bool is_trigger) {
		m_is_trigger = is_trigger;
		SendUpdateEvent(); // Shape needs recreating
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
		if (m_body_type == DYNAMIC) {
			auto vec = ((PxRigidDynamic*)p_rigid_actor)->getAngularVelocity();
			return glm::vec3(vec.x, vec.y, vec.z);
		}
		else
		{
			return { 0, 0, 0 };
		}
	}


	void PhysicsComponent::SetMaterial(PhysXMaterialAsset& material) {
		p_material->p_material->release();
		p_material = &material;
		material.p_material->acquireReference();
		SendUpdateEvent();
	}

	glm::vec3 PhysicsComponent::GetVelocity() const {
		if (m_body_type == DYNAMIC) {
			auto vec = ((PxRigidDynamic*)p_rigid_actor)->getLinearVelocity();
			return glm::vec3(vec.x, vec.y, vec.z);
		}
		else {
			return { 0, 0, 0 };
		}
	}

	void PhysicsComponent::SendUpdateEvent() {
		Events::ECS_Event<PhysicsComponent> phys_event{ Events::ECS_EventType::COMP_UPDATED, this };
		Events::EventManager::DispatchEvent(phys_event);
	}



	void CharacterControllerComponent::Move(glm::vec3 disp, float minDist, float elapsedTime) {
		p_controller->move(PxVec3(disp.x, disp.y, disp.z), minDist, elapsedTime, 0);

		moved_during_frame = true;
	}
}