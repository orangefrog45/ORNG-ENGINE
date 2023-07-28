#include "pch/pch.h"
#include "components/ComponentSystems.h"
#include "physics/Physics.h"
#include "scene/SceneEntity.h"
#include "glm/glm/gtc/quaternion.hpp"
#include "events/EventManager.h"


namespace ORNG {
	using namespace physx;


	static void OnComponentAdd(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<PhysicsComponent>(registry, entity, Events::ECS_EventType::COMP_ADDED);
	}

	static void OnComponentDestroy(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<PhysicsComponent>(registry, entity, Events::ECS_EventType::COMP_DELETED);
	}


	PhysicsSystem::PhysicsSystem(entt::registry* p_registry) : mp_registry(p_registry) {
		// Initialize event listeners
		// Physics listener
		m_phys_listener.OnEvent = [this](const Events::ECS_Event<PhysicsComponent>& t_event) {
			switch (t_event.event_type) {
			case Events::ECS_EventType::COMP_ADDED: // Only doing index 0 because these events will only ever affect a single component currently
				InitComponent(t_event.affected_components[0]);
				break;
			case Events::ECS_EventType::COMP_UPDATED:
				UpdateComponentState(t_event.affected_components[0]);
				break;
			case Events::ECS_EventType::COMP_DELETED:
				RemoveComponent(t_event.affected_components[0]);
				break;
			}
		};

		// Transform update listener
		m_transform_listener.OnEvent = [this](const Events::ECS_Event<TransformComponent>& t_event) {
			if (t_event.event_type == Events::ECS_EventType::COMP_UPDATED) {
				auto& transform = *t_event.affected_components[0];
				auto* p_phys_comp = transform.GetEntity()->GetComponent<PhysicsComponent>();

				if (!p_phys_comp)
					return;

				auto n_transforms = transform.GetAbsoluteTransforms();
				glm::vec3 abs_pos = n_transforms[0];
				glm::quat n_quat{glm::radians(n_transforms[2])};


				PxVec3 px_pos{ abs_pos.x, abs_pos.y, abs_pos.z };
				PxQuat n_px_quat{ n_quat.x, n_quat.y, n_quat.z, n_quat.w };
				PxTransform px_transform{ px_pos, n_px_quat };

				p_phys_comp->p_rigid_actor->setGlobalPose(px_transform);

				if (t_event.sub_event_type == TransformComponent::UpdateType::SCALE || t_event.sub_event_type == TransformComponent::UpdateType::ALL) { // Whole shape needs to be rebuilt
					p_phys_comp->UpdateGeometry(p_phys_comp->geometry_type);
					return;
				}
			}
		};

		Events::EventManager::RegisterListener(m_phys_listener);
		Events::EventManager::RegisterListener(m_transform_listener);


	};


	void PhysicsSystem::OnLoad() {

		PxBroadPhaseDesc bpDesc(PxBroadPhaseType::eABP);

		mp_broadphase = PxCreateBroadPhase(bpDesc);
		mp_aabb_manager = PxCreateAABBManager(*mp_broadphase);

		PxSceneDesc scene_desc{ Physics::GetToleranceScale() };
		scene_desc.filterShader = PxDefaultSimulationFilterShader;
		scene_desc.gravity = PxVec3(0, -9.81, 0);
		scene_desc.cpuDispatcher = Physics::GetCPUDispatcher();
		scene_desc.cudaContextManager = Physics::GetCudaContextManager();
		scene_desc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
		scene_desc.flags |= PxSceneFlag::eENABLE_PCM;
		scene_desc.broadPhaseType = PxBroadPhaseType::eGPU;
		PxCudaContextManagerDesc desc;


		mp_scene = Physics::GetPhysics()->createScene(scene_desc);

		m_physics_materials.push_back(Physics::GetPhysics()->createMaterial(0.25f, 0.1f, 0.1f));
		m_physics_materials[0]->acquireReference();

		mp_registry->on_construct<PhysicsComponent>().connect<&OnComponentAdd>();
		mp_registry->on_destroy<PhysicsComponent>().connect<&OnComponentDestroy>();

	}


	void PhysicsSystem::RemoveComponent(PhysicsComponent* p_comp) {
		mp_scene->removeActor(*p_comp->p_rigid_actor);
		p_comp->p_rigid_actor->release();

		p_comp->p_shape->release();
		p_comp->p_material->release();
	}


	void PhysicsSystem::OnUnload() {

		for (auto* p_material : m_physics_materials) {
			p_material->release();
		}
		m_physics_materials.clear();

		mp_scene->release();
		mp_aabb_manager->release();
		PX_RELEASE(mp_broadphase);
	}



	PxTriangleMesh* PhysicsSystem::GetOrCreateTriangleMesh(const MeshAsset* p_mesh_asset) {
		if (m_triangle_meshes.contains(p_mesh_asset))
			return m_triangle_meshes[p_mesh_asset];



		const VAO& vao = p_mesh_asset->GetVAO();
		PxCookingParams params(Physics::GetToleranceScale());
		params.buildGPUData = true;
		// disable mesh cleaning - perform mesh validation on development configurations
		params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;
		// disable edge precompute, edges are set for each triangle, slows contact generation
		params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;
		// lower hierarchy for internal mesh
		//params.meshCookingHint = PxMeshCookingHint::eCOOKING_PERFORMANCE;

		PxSDFDesc desc = PxSDFDesc();

		desc.subgridSize = 4;
		desc.spacing = 5;
		PxTriangleMeshDesc meshDesc;
		meshDesc.points.count = vao.vertex_data.positions.size();
		meshDesc.points.stride = sizeof(glm::vec3);
		meshDesc.points.data = (void*)&vao.vertex_data.positions[0];

		meshDesc.triangles.count = vao.vertex_data.indices.size() / 3;
		meshDesc.triangles.stride = 3 * sizeof(unsigned int);
		meshDesc.triangles.data = (void*)&vao.vertex_data.indices[0];
		meshDesc.sdfDesc = &desc;
#ifdef _DEBUG
		// mesh should be validated before cooked without the mesh cleaning
		bool res = PxValidateTriangleMesh(params, meshDesc);
		PX_ASSERT(res);
#endif

		PxTriangleMesh* aTriangleMesh = PxCreateTriangleMesh(params, meshDesc, Physics::GetPhysics()->getPhysicsInsertionCallback());
		m_triangle_meshes[p_mesh_asset] = aTriangleMesh;
		return aTriangleMesh;
	}



	void PhysicsSystem::UpdateComponentState(PhysicsComponent* p_comp) {
		// Update component geometry
		const auto* p_mesh_comp = p_comp->GetEntity()->GetComponent<MeshComponent>();
		const AABB& aabb = p_mesh_comp ? p_mesh_comp->GetMeshData()->GetAABB() : AABB(glm::vec3(-1), glm::vec3(1));

		glm::vec3 scale_factor = p_comp->GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[1];
		glm::vec3 scaled_extents = aabb.max * scale_factor;
		float radius = glm::max(glm::max(scaled_extents.x, scaled_extents.y), scaled_extents.z);


		switch (p_comp->geometry_type) {
		case PhysicsComponent::SPHERE:
			p_comp->p_shape->release();
			p_comp->p_shape = Physics::GetPhysics()->createShape(PxSphereGeometry(radius), *p_comp->p_material);
			break;
		case PhysicsComponent::BOX:
			p_comp->p_shape->release();
			p_comp->p_shape = Physics::GetPhysics()->createShape(PxBoxGeometry(scaled_extents.x, scaled_extents.y, scaled_extents.z), *p_comp->p_material);
			break;
		case PhysicsComponent::TRIANGLE_MESH:
			if (!p_mesh_comp)
				return;

			p_comp->p_shape->release();
			PxTriangleMesh* aTriangleMesh = GetOrCreateTriangleMesh(p_mesh_comp->GetMeshData());
			p_comp->p_shape = Physics::GetPhysics()->createShape(PxTriangleMeshGeometry(aTriangleMesh, PxMeshScale(PxVec3(scale_factor.x, scale_factor.y, scale_factor.z))), *p_comp->p_material);
			break;
		}

		p_comp->p_shape->acquireReference();

		// Update rigid body type
		PxTransform current_transform = p_comp->p_rigid_actor->getGlobalPose();
		p_comp->p_rigid_actor->release();

		if (p_comp->rigid_body_type == PhysicsComponent::STATIC) {
			p_comp->p_rigid_actor = PxCreateStatic(*Physics::GetPhysics(), current_transform, *p_comp->p_shape);
		}
		else if (p_comp->rigid_body_type == PhysicsComponent::DYNAMIC) {
			p_comp->p_rigid_actor = PxCreateDynamic(*Physics::GetPhysics(), current_transform, *p_comp->p_shape, 1.f);
		}

		mp_scene->addActor(*p_comp->p_rigid_actor);
	}


	void PhysicsSystem::InitComponent(PhysicsComponent* p_comp) {
		const auto* p_meshc = p_comp->GetEntity()->GetComponent<MeshComponent>();
		const auto* p_transform = p_comp->GetEntity()->GetComponent<TransformComponent>();
		auto transforms = p_transform->GetAbsoluteTransforms();
		glm::vec3 extents = p_transform->GetScale() * (p_meshc ? p_meshc->GetMeshData()->GetAABB().max : glm::vec3(1));

		glm::vec3 pos = transforms[0];
		glm::vec3 rot = transforms[2];


		glm::quat quat = glm::quat(glm::radians(rot));
		PxQuat px_quat{ quat.x, quat.y, quat.z, quat.w };

		// Create default material
		p_comp->p_material = m_physics_materials[0];
		p_comp->p_material->acquireReference();

		// Setup shape based on mesh AABB if available
		p_comp->p_shape = Physics::GetPhysics()->createShape(PxBoxGeometry(extents.x, extents.y, extents.z), *p_comp->p_material);
		p_comp->p_shape->acquireReference();

		// Static by default
		p_comp->p_rigid_actor = PxCreateStatic(*Physics::GetPhysics(), PxTransform(PxVec3(pos.x, pos.y, pos.z), px_quat), *p_comp->p_shape);

		mp_scene->addActor(*p_comp->p_rigid_actor);
		p_comp->rigid_body_type = PhysicsComponent::STATIC;

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

		auto view = mp_registry->view<PhysicsComponent, TransformComponent>();
		for (auto [entity, phys, transform] : view.each()) {
			if (phys.rigid_body_type == PhysicsComponent::STATIC || static_cast<PxRigidDynamic*>(phys.p_rigid_actor)->isSleeping())
				continue;


			PxTransform px_transform = phys.p_rigid_actor->getGlobalPose();
			PxVec3 phys_pos = px_transform.p;
			PxQuatT phys_rot = px_transform.q;

			// Deltas used to get around transform inheritance, if set to absolute transform then transforms would be inherited twice
			glm::quat phys_quat = glm::quat(phys_rot.w, phys_rot.x, phys_rot.y, phys_rot.z);
			glm::vec3 orientation = glm::degrees(glm::eulerAngles(phys_quat));

			transform.SetAbsolutePosition(glm::vec3(phys_pos.x, phys_pos.y, phys_pos.z));
			transform.SetAbsoluteOrientation(orientation);
		}
	}







}