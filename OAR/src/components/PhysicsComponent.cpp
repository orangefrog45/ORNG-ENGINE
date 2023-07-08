#include "pch/pch.h"
#include "components/PhysicsComponent.h"
#include "physics/Physics.h"
#include "scene/SceneEntity.h"
#include "glm/glm/gtc/quaternion.hpp"
#include "components/ComponentManagers.h"

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

		p_shape = Physics::GetPhysics()->createShape(PxBoxGeometry(half_extents.x, half_extents.y, half_extents.z), *p_material);
		p_shape->acquireReference();

		if (type == DYNAMIC)
			p_rigid_actor = PxCreateDynamic(*Physics::GetPhysics(), PxTransform(PxVec3(pos.x, pos.y, pos.z), px_quat), *p_shape, 1.f);
		else
			p_rigid_actor = PxCreateStatic(*Physics::GetPhysics(), PxTransform(PxVec3(pos.x, pos.y, pos.z), px_quat), *p_shape);

		p_scene->addActor(*p_rigid_actor);
		rigid_body_type = type;
	}



	void PhysicsComponent::SetBodyType(RigidBodyType type) {
		PxTransform current_transform = p_rigid_actor->getGlobalPose();
		p_scene->removeActor(*p_rigid_actor);
		p_rigid_actor->release();

		if (type == STATIC) {
			p_rigid_actor = PxCreateStatic(*Physics::GetPhysics(), current_transform, *p_shape);
			rigid_body_type = STATIC;
		}
		else if (type == DYNAMIC) {
			p_rigid_actor = PxCreateDynamic(*Physics::GetPhysics(), current_transform, *p_shape, 1.f);
			rigid_body_type = DYNAMIC;
		}
		p_scene->addActor(*p_rigid_actor);
	}


	void PhysicsComponent::UpdateGeometry(GeometryType type) {
		auto* p_mesh_comp = GetEntity()->GetComponent<MeshComponent>();
		const AABB& aabb = p_mesh_comp ? p_mesh_comp->GetMeshData()->GetAABB() : AABB(glm::vec3(-1), glm::vec3(1));
		glm::vec3 scale_factor = mp_transform->GetAbsoluteTransforms()[1];
		glm::vec3 scaled_extents = aabb.max * scale_factor;
		float radius = glm::max(glm::max(scaled_extents.x, scaled_extents.y), scaled_extents.z);


		switch (type) {
		case SPHERE:
			p_shape->release();
			p_shape = Physics::GetPhysics()->createShape(PxSphereGeometry(radius), *p_material);
			break;
		case BOX:
			p_shape->release();
			p_shape = Physics::GetPhysics()->createShape(PxBoxGeometry(scaled_extents.x, scaled_extents.y, scaled_extents.z), *p_material);
			break;
		case TRIANGLE_MESH:
			auto* p_mesh = GetEntity()->GetComponent<MeshComponent>();

			if (!p_mesh)
				return;

			p_shape->release();
			PxTriangleMesh* aTriangleMesh = mp_system->GetOrCreateTriangleMesh(p_mesh->GetMeshData());
			p_shape = Physics::GetPhysics()->createShape(PxTriangleMeshGeometry(aTriangleMesh, PxMeshScale(PxVec3(scale_factor.x, scale_factor.y, scale_factor.z))), *p_material);
			break;
		}

		p_shape->acquireReference();
		// Has to be set to static with triangle meshes as currently don't support dynamic triangle meshes
		SetBodyType(type == TRIANGLE_MESH ? STATIC : rigid_body_type);

		geometry_type = type;
	}


	PhysicsComponent::~PhysicsComponent() {
		p_scene->removeActor(*p_rigid_actor);
		p_rigid_actor->release();

		p_shape->release();
		p_material->release();
	}
}