#include "pch/pch.h"
#include "components/PhysicsComponent.h"
#include "physics/Physics.h"
#include "scene/SceneEntity.h"
#include "glm/glm/gtc/quaternion.hpp"

namespace ORNG {

	void PhysicsComponent::Init(PxScene* t_scene, glm::vec3 half_extents, RigidBodyType type, PxMaterial* t_p_material) {
		p_scene = t_scene;
		auto transforms = mp_transform->GetAbsoluteTransforms();

		glm::vec3 pos = transforms[0];
		glm::vec3 rot = transforms[2];

		old_pos = pos;
		old_orientation = rot;

		glm::quat quat = glm::quat(glm::radians(rot));
		PxQuat px_quat{ quat.x, quat.y, quat.z, quat.w };

		// Create default material
		p_material = t_p_material;
		p_material->acquireReference();

		if (type == DYNAMIC) {
			p_shape = Physics::GetPhysics()->createShape(PxBoxGeometry(half_extents.x, half_extents.y, half_extents.z), *p_material);
			p_shape->acquireReference();
			p_rigid_dynamic = PxCreateDynamic(*Physics::GetPhysics(), PxTransform(PxVec3(pos.x, pos.y, pos.z), px_quat), *p_shape, 1.f);
			p_scene->addActor(*p_rigid_dynamic);
		}
		else {
			p_shape = Physics::GetPhysics()->createShape(PxBoxGeometry(half_extents.x, half_extents.y, half_extents.z), *p_material);
			p_shape->acquireReference();
			p_rigid_static = PxCreateStatic(*Physics::GetPhysics(), PxTransform(PxVec3(pos.x, pos.y, pos.z), px_quat), *p_shape);
			p_scene->addActor(*p_rigid_static);
		}
		rigid_body_type = type;
	}

	void PhysicsComponent::SetBodyType(RigidBodyType type) {
		if (rigid_body_type == DYNAMIC && type == STATIC) {
			PxTransform current_transform = p_rigid_dynamic->getGlobalPose();
			p_scene->removeActor(*p_rigid_dynamic);
			p_rigid_dynamic->release();
			p_rigid_dynamic = nullptr;

			p_rigid_static = PxCreateStatic(*Physics::GetPhysics(), current_transform, *p_shape);
			p_scene->addActor(*p_rigid_static);

			rigid_body_type = STATIC;
		}
		else if (rigid_body_type == STATIC && type == DYNAMIC) {
			PxTransform current_transform = p_rigid_static->getGlobalPose();
			p_scene->removeActor(*p_rigid_static);
			p_rigid_static->release();
			p_rigid_static = nullptr;

			p_rigid_dynamic = PxCreateDynamic(*Physics::GetPhysics(), current_transform, *p_shape, 1.f);
			p_scene->addActor(*p_rigid_dynamic);

			rigid_body_type = DYNAMIC;
		}
	}


	void PhysicsComponent::UpdateGeometry(GeometryType type) {

		const AABB& aabb = GetEntity()->GetComponent<MeshComponent>()->GetMeshData()->GetAABB();
		glm::vec3 scale_factor = GetEntity()->GetComponent<TransformComponent>()->GetScale();
		glm::vec3 scaled_extents = aabb.max * scale_factor;

		if (type == SPHERE) {
			float radius = glm::max(glm::max(scaled_extents.x, scaled_extents.y), scaled_extents.z);
			p_shape->release();
			p_shape = Physics::GetPhysics()->createShape(PxSphereGeometry(radius), *p_material);
		}
		else {
			p_shape->release();
			p_shape = Physics::GetPhysics()->createShape(PxBoxGeometry(scaled_extents.x, scaled_extents.y, scaled_extents.z), *p_material);
		}

		if (rigid_body_type == DYNAMIC) {
			PxTransform prev_transform = p_rigid_dynamic->getGlobalPose();
			p_scene->removeActor(*p_rigid_dynamic);
			p_rigid_dynamic->release();
			p_rigid_dynamic = PxCreateDynamic(*Physics::GetPhysics(), prev_transform, *p_shape, 1.f);
			p_scene->addActor(*p_rigid_dynamic);
		}
		else {
			PxTransform prev_transform = p_rigid_static->getGlobalPose();
			p_scene->removeActor(*p_rigid_static);
			p_rigid_static->release();
			p_rigid_static = PxCreateStatic(*Physics::GetPhysics(), prev_transform, *p_shape);
			p_scene->addActor(*p_rigid_static);
		}

		geometry_type = type;
	}


	PhysicsComponent::~PhysicsComponent() {
		if (p_rigid_dynamic) {
			p_scene->removeActor(*p_rigid_dynamic);
			p_rigid_dynamic->release();
		}
		else if (p_rigid_static) {
			p_scene->removeActor(*p_rigid_static);
			p_rigid_static->release();
		}

		p_shape->release();
		p_material->release();
	}
}