#include "pch/pch.h"
#include "components/ComponentManagers.h"
#include "physics/Physics.h"
#include "scene/SceneEntity.h"
#include "glm/glm/gtc/quaternion.hpp"


namespace ORNG {

	void PhysicsSystem::OnLoad() {
		const PxU32 num_threads = 8;
		mp_dispatcher = PxDefaultCpuDispatcherCreate(num_threads);
		PxBroadPhaseDesc bpDesc(PxBroadPhaseType::eABP);

		mp_broadphase = PxCreateBroadPhase(bpDesc);
		mp_aabb_manager = PxCreateAABBManager(*mp_broadphase);

		PxSceneDesc scene_desc{ Physics::GetToleranceScale() };
		scene_desc.filterShader = PxDefaultSimulationFilterShader;
		scene_desc.broadPhaseType = PxBroadPhaseType::eABP;
		scene_desc.gravity = PxVec3(0, -9.81, 0);
		scene_desc.cpuDispatcher = mp_dispatcher;
		PxCudaContextManagerDesc desc;

		mp_scene = Physics::GetPhysics()->createScene(scene_desc);

		m_physics_materials.push_back(Physics::GetPhysics()->createMaterial(0.25f, 0.1f, 0.1f));
		m_physics_materials[0]->acquireReference();
	}

	void PhysicsSystem::DeleteComponent(SceneEntity* p_entity) {
		auto it = std::find_if(m_physics_components.begin(), m_physics_components.end(), [=](auto* p_comp) {return p_comp->GetEntityHandle() == p_entity->GetID(); });

		if (it == m_physics_components.end()) {
			OAR_CORE_ERROR("No physics component found in entity '{0}', not deleted.", p_entity->name);
			return;
		}

		auto* p_transform = p_entity->GetComponent<TransformComponent>();
		p_transform->update_callbacks.erase(TransformComponent::CallbackType::PHYSICS);

		delete* it;
		m_physics_components.erase(it);
	}

	void PhysicsSystem::OnUnload() {
		for (auto* p_comp : m_physics_components) {
			auto* p_transform = p_comp->mp_transform;
			p_transform->update_callbacks.erase(TransformComponent::CallbackType::PHYSICS);
			delete p_comp;
		}

		m_physics_components.clear();

		for (auto* p_material : m_physics_materials) {
			p_material->release();
		}
		mp_scene->release();
		PX_RELEASE(mp_broadphase);
	}



	void PhysicsSystem::OnUpdate(float ts) {

		if (m_physics_paused)
			return;

		m_accumulator += ts;

		if (m_accumulator < m_step_size)
			return;

		m_accumulator -= m_step_size;
		mp_scene->simulate(m_step_size);
		mp_scene->fetchResults(true);

		for (auto* p_comp : m_physics_components) {
			if (p_comp->rigid_body_type == PhysicsComponent::STATIC || p_comp->p_rigid_dynamic->isSleeping())
				continue;


			PxTransform transform = p_comp->p_rigid_dynamic->getGlobalPose();
			PxVec3 phys_pos = transform.p;
			PxQuatT phys_rot = transform.q;

			glm::vec3 delta_translation = glm::vec3(phys_pos.x, phys_pos.y, phys_pos.z) - p_comp->old_pos;
			glm::quat phys_quat = glm::quat(phys_rot.w, phys_rot.x, phys_rot.y, phys_rot.z);
			glm::vec3 delta_rotation = glm::degrees(glm::eulerAngles(phys_quat)) - p_comp->old_orientation;

			p_comp->mp_transform->SetPosition(p_comp->mp_transform->GetPosition() + delta_translation);
			p_comp->mp_transform->SetOrientation(p_comp->mp_transform->GetRotation() + delta_rotation);

		}
	}





	PhysicsComponent* PhysicsSystem::GetComponent(uint64_t entity_id) {
		auto it = std::find_if(m_physics_components.begin(), m_physics_components.end(), [&](const auto& p_comp) {return p_comp->GetEntityHandle() == entity_id; });
		return it == m_physics_components.end() ? nullptr : *it;
	}




	PhysicsComponent* PhysicsSystem::AddComponent(SceneEntity* p_entity, PhysicsComponent::RigidBodyType type) {


		if (auto* p_found_comp = GetComponent(p_entity->GetID())) {
			OAR_CORE_ERROR("Physics component not added to component '{0}', already has one", p_entity->name);
			return p_found_comp;
		}
		auto* meshc = p_entity->GetComponent<MeshComponent>();

		if (!meshc) {
			OAR_CORE_ERROR("No mesh component found for physics component to attach to in entity '{0}', physics component not added", p_entity->name);
			return nullptr;
		}



		auto* p_transform = p_entity->GetComponent<TransformComponent>();
		auto* p_comp = new PhysicsComponent(p_entity, p_transform);
		m_physics_components.push_back(p_comp);


		const AABB& aabb = meshc->GetMeshData()->GetAABB();
		p_comp->Init(mp_scene, p_transform->GetScale() * aabb.max, type, m_physics_materials[0]);


		p_transform->update_callbacks[TransformComponent::CallbackType::PHYSICS] = ([p_comp, p_transform](TransformComponent::UpdateType t_update_type) {

			auto transforms = p_transform->GetAbsoluteTransforms();
			glm::vec3 pos = transforms[0];
			glm::quat quat{glm::radians(transforms[2])};


			PxVec3 px_pos{ pos.x, pos.y, pos.z };
			PxQuat px_quat{ quat.x, quat.y, quat.z, quat.w };
			PxTransform transform{ px_pos, px_quat };

			if (p_comp->rigid_body_type == PhysicsComponent::STATIC)
				p_comp->p_rigid_static->setGlobalPose(transform);
			else
				p_comp->p_rigid_dynamic->setGlobalPose(transform);

			p_comp->old_orientation = glm::degrees(glm::eulerAngles(quat));
			p_comp->old_pos = pos;

			if (t_update_type == TransformComponent::UpdateType::SCALE || t_update_type == TransformComponent::UpdateType::ALL) { // Whole shape needs to be rebuilt
				p_comp->UpdateGeometry(p_comp->geometry_type);
				return;
			}

			});

		return p_comp;
	}

}