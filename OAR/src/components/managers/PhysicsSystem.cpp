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
	}

	void PhysicsSystem::DeleteComponent(SceneEntity* p_entity) {
		auto it = std::find_if(m_physics_components.begin(), m_physics_components.end(), [&](auto& p_comp) {return p_comp->GetEntityHandle() == p_entity->GetID(); });

		if (it == m_physics_components.end()) {
			OAR_CORE_ERROR("No physics component found in entity '{0}', not deleted.", p_entity->name);
			return;
		}

		auto* p_comp = *it;

		auto* p_transform = p_entity->GetComponent<TransformComponent>();
		p_transform->update_callbacks.erase(TransformComponent::CallbackType::PHYSICS);

		mp_scene->removeActor(*p_comp->p_rigid_dynamic);
		delete* it;

		m_physics_components.erase(it);
	}

	void PhysicsSystem::OnUnload() {
		for (auto* p_comp : m_physics_components) {
			DeleteComponent(p_comp->GetEntity());
		}

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

			auto* p_transform = p_comp->GetEntity()->GetComponent<TransformComponent>();
			PxVec3 pos = p_comp->p_rigid_dynamic->getGlobalPose().p;
			PxQuatT rot = p_comp->p_rigid_dynamic->getGlobalPose().q;
			p_transform->SetPosition(pos.x, pos.y, pos.z);
			glm::quat quat = glm::quat(rot.w, rot.x, rot.y, rot.z);
			p_transform->SetOrientation(glm::degrees(glm::eulerAngles(quat)));
		}
	}





	PhysicsComponent* PhysicsSystem::GetComponent(unsigned long entity_id) {
		auto it = std::find_if(m_physics_components.begin(), m_physics_components.end(), [&](const auto& p_comp) {return p_comp->GetEntityHandle() == entity_id; });
		return it == m_physics_components.end() ? nullptr : *it;
	}




	PhysicsComponent* PhysicsSystem::AddComponent(SceneEntity* p_entity, PhysicsComponent::RigidBodyType type) {
		if (GetComponent(p_entity->GetID())) {
			OAR_CORE_ERROR("Physics component not added to component '{0}', already has one", p_entity->name);
			return nullptr;
		}
		auto* meshc = p_entity->GetComponent<MeshComponent>();

		if (!meshc) {
			OAR_CORE_ERROR("No mesh component found for physics component to attach to in entity '{0}', physics component not added", p_entity->name);
			return nullptr;
		}



		auto* p_comp = new PhysicsComponent(p_entity);
		m_physics_components.push_back(p_comp);

		auto* p_transform = p_entity->GetComponent<TransformComponent>();

		const AABB& aabb = meshc->GetMeshData()->GetAABB();
		p_comp->Init(mp_scene, p_transform->GetScale() * aabb.max, 0.5f, 0.5f, 0.25f, type);


		p_transform->update_callbacks[TransformComponent::CallbackType::PHYSICS] = ([p_comp, p_transform] {
			glm::vec3 pos {p_transform->GetPosition()};
			glm::quat quat{glm::radians(p_transform->GetRotation())};

			PxVec3 px_pos{ pos.x, pos.y, pos.z };
			PxQuat px_quat{ quat.x, quat.y, quat.z, quat.w };
			PxTransform transform{ px_pos, px_quat };
			if (p_comp->rigid_body_type == PhysicsComponent::STATIC) {
				p_comp->p_rigid_static->setGlobalPose(transform);
			}
			else {
				p_comp->p_rigid_dynamic->setGlobalPose(transform);
			}
			});


		return p_comp;
	}

}