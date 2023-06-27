#include "pch/pch.h"
#include "components/PhysicsComponent.h"
#include "physics/Physics.h"
#include "scene/SceneEntity.h"
#include "glm/glm/gtc/quaternion.hpp"

namespace ORNG {

	void PhysicsComponent::Init(PxScene* t_scene, glm::vec3 half_extents, float static_friction, float dynamic_friction, float restitution, RigidBodyType type) {
		p_scene = t_scene;
		auto transforms = GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms();

		glm::vec3 pos = transforms[0];
		glm::vec3 rot = transforms[2];

		glm::quat quat = glm::quat(glm::radians(rot));
		PxQuat px_quat{ quat.x, quat.y, quat.z, quat.w };
		p_material = Physics::GetPhysics()->createMaterial(static_friction, dynamic_friction, restitution);

		if (type == DYNAMIC) {
			p_shape = Physics::GetPhysics()->createShape(PxBoxGeometry(half_extents.x, half_extents.y, half_extents.z), *p_material);
			p_rigid_dynamic = PxCreateDynamic(*Physics::GetPhysics(), PxTransform(PxVec3(pos.x, pos.y, pos.z), px_quat), *p_shape, 1.f);
			p_scene->addActor(*p_rigid_dynamic);
		}
		else {
			p_shape = Physics::GetPhysics()->createShape(PxBoxGeometry(half_extents.x, half_extents.y, half_extents.z), *p_material);
			p_rigid_static = PxCreateStatic(*Physics::GetPhysics(), PxTransform(PxVec3(pos.x, pos.y, pos.z), px_quat), *p_shape);
			p_scene->addActor(*p_rigid_static);
		}


		rigid_body_type = type;
	}


	void PhysicsComponent::SetGeometryType(GeometryType type) {

		const AABB& aabb = GetEntity()->GetComponent<MeshComponent>()->GetMeshData()->GetAABB();
		glm::vec3 scale_factor = GetEntity()->GetComponent<TransformComponent>()->GetScale();
		glm::vec3 scaled_extents = aabb.max * scale_factor;

		PxTransform prev_transform = p_rigid_dynamic->getGlobalPose();

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
			p_scene->removeActor(*p_rigid_dynamic);
			p_rigid_dynamic->release();
			p_rigid_dynamic = PxCreateDynamic(*Physics::GetPhysics(), prev_transform, *p_shape, 1.f);
			p_scene->addActor(*p_rigid_dynamic);
		}
		else {
			p_scene->removeActor(*p_rigid_static);
			p_rigid_static->release();
			p_rigid_static = PxCreateStatic(*Physics::GetPhysics(), prev_transform, *p_shape);
			p_scene->addActor(*p_rigid_static);
		}


	}


	PhysicsComponent::~PhysicsComponent() {
		if (p_rigid_dynamic)
			p_rigid_dynamic->release();
		else if (p_rigid_static)
			p_rigid_static->release();

		p_shape->release();
		p_material->release();
	}
}