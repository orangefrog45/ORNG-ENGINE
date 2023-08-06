#include "pch/pch.h"
#include "components/ComponentSystems.h"
#include "physics/Physics.h"
#include "scene/SceneEntity.h"
#include "glm/glm/gtc/quaternion.hpp"
#include "events/EventManager.h"


namespace ORNG {
	using namespace physx;


	static void OnPhysComponentAdd(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<PhysicsComponent>(registry, entity, Events::ECS_EventType::COMP_ADDED);
	}

	static void OnPhysComponentDestroy(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<PhysicsComponent>(registry, entity, Events::ECS_EventType::COMP_DELETED);
	}

	static void OnCharacterControllerComponentAdd(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<CharacterControllerComponent>(registry, entity, Events::ECS_EventType::COMP_ADDED);
	}

	static void OnCharacterControllerComponentDestroy(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<CharacterControllerComponent>(registry, entity, Events::ECS_EventType::COMP_DELETED);
	}

	static PxTransform TransformComponentToPxTransform(const TransformComponent& transform) {
		auto abs_transforms = transform.GetAbsoluteTransforms();
		glm::quat quat{abs_transforms[2]};

		return PxTransform{ { abs_transforms[0].x, abs_transforms[0].y, abs_transforms[0].z }, { quat.x, quat.y, quat.z, quat.w } };
	}


	PhysicsSystem::PhysicsSystem(entt::registry* p_registry, uint64_t scene_uuid) : mp_registry(p_registry), ComponentSystem(scene_uuid) {
		// Initialize event listeners
		// Physics listener
		ORNG_CORE_ERROR("PHYS {0}", scene_uuid);

		m_phys_listener.scene_id = scene_uuid;
		m_phys_listener.OnEvent = [this](const Events::ECS_Event<PhysicsComponent>& t_event) {
			switch (t_event.event_type) {
				using enum Events::ECS_EventType;
			case COMP_ADDED: // Only doing index 0 because these events will only ever affect a single component currently
				InitComponent(t_event.affected_components[0]);
				break;
			case COMP_UPDATED:
				UpdateComponentState(t_event.affected_components[0]);
				break;
			case COMP_DELETED:
				RemoveComponent(t_event.affected_components[0]);
				break;
			}
		};

		// Character controller listener
		m_character_controller_listener.scene_id = scene_uuid;
		m_character_controller_listener.OnEvent = [this](const Events::ECS_Event<CharacterControllerComponent>& t_event) {
			using enum Events::ECS_EventType;
			switch (t_event.event_type) {
				using enum Events::ECS_EventType;
			case COMP_ADDED: // Only doing index 0 because these events will only ever affect a single component currently
				InitComponent(t_event.affected_components[0]);
				break;
			case COMP_DELETED:
				RemoveComponent(t_event.affected_components[0]);
				break;
			}
		};

		// Transform update listener
		m_transform_listener.scene_id = scene_uuid;
		m_transform_listener.OnEvent = [this](const Events::ECS_Event<TransformComponent>& t_event) {
			if (t_event.event_type == Events::ECS_EventType::COMP_UPDATED) {
				auto* p_transform = t_event.affected_components[0];
				auto* p_phys_comp = p_transform->GetEntity()->GetComponent<PhysicsComponent>();

				if (p_transform == mp_currently_updating_transform) // Ignore transform event if it was updated by the physics engine, as the states are already synced
					return;

				if (auto* p_controller_comp = p_transform->GetEntity()->GetComponent<CharacterControllerComponent>()) {
					glm::vec3 pos = p_transform->GetAbsoluteTransforms()[0];
					p_controller_comp->mp_controller->setPosition({ pos.x, pos.y, pos.z });
				}

				if (p_phys_comp) {

					if (t_event.sub_event_type == TransformComponent::UpdateType::SCALE || t_event.sub_event_type == TransformComponent::UpdateType::ALL) { // Whole shape needs to be rebuilt
						UpdateComponentState(p_phys_comp);
						return;
					}

					auto n_transforms = p_transform->GetAbsoluteTransforms();
					glm::vec3 abs_pos = n_transforms[0];
					glm::quat n_quat{glm::radians(n_transforms[2])};

					PxVec3 px_pos{ abs_pos.x, abs_pos.y, abs_pos.z };
					PxQuat n_px_quat{ n_quat.x, n_quat.y, n_quat.z, n_quat.w };
					PxTransform px_transform{ px_pos, n_px_quat };

					p_phys_comp->p_rigid_actor->setGlobalPose(px_transform);
				}

			}
		};

		Events::EventManager::RegisterListener(m_phys_listener);
		Events::EventManager::RegisterListener(m_character_controller_listener);
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
		mp_controller_manager = PxCreateControllerManager(*mp_scene);
		m_physics_materials.push_back(Physics::GetPhysics()->createMaterial(0.25f, 0.1f, 0.1f));
		m_physics_materials[0]->acquireReference();

		mp_registry->on_construct<PhysicsComponent>().connect<&OnPhysComponentAdd>();
		mp_registry->on_destroy<PhysicsComponent>().connect<&OnPhysComponentDestroy>();
		mp_registry->on_construct<CharacterControllerComponent>().connect<&OnCharacterControllerComponentAdd>();
		mp_registry->on_destroy<CharacterControllerComponent>().connect<&OnCharacterControllerComponentDestroy>();

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

		mp_controller_manager->release();
		mp_scene->release();
		mp_aabb_manager->release();
		PX_RELEASE(mp_broadphase);
	}



	PxTriangleMesh* PhysicsSystem::GetOrCreateTriangleMesh(const MeshAsset* p_mesh_asset) {
		if (m_triangle_meshes.contains(p_mesh_asset))
			return m_triangle_meshes[p_mesh_asset];



		const VAO& vao = p_mesh_asset->GetVAO();
		PxCookingParams params(Physics::GetToleranceScale());
		//params.buildGPUData = true;
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


		switch (p_comp->geometry_type) {
		case PhysicsComponent::SPHERE:
			p_comp->p_shape->release();
			p_comp->p_shape = Physics::GetPhysics()->createShape(PxSphereGeometry(glm::max(glm::max(scaled_extents.x, scaled_extents.y), scaled_extents.z)), *p_comp->p_material);
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

	static void UpdateTransformCompFromGlobalPose(const PxTransform& pose, TransformComponent& transform) {
		PxVec3 phys_pos = pose.p;
		PxQuatT phys_rot = pose.q;

		glm::quat phys_quat = glm::quat(phys_rot.w, phys_rot.x, phys_rot.y, phys_rot.z);
		glm::vec3 orientation = glm::degrees(glm::eulerAngles(phys_quat));


		transform.SetAbsolutePosition(glm::vec3(phys_pos.x, phys_pos.y, phys_pos.z));
		transform.SetAbsoluteOrientation(orientation);
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

		for (auto [entity, phys, transform] : mp_registry->view<PhysicsComponent, TransformComponent>().each()) {
			if (phys.rigid_body_type == PhysicsComponent::STATIC || static_cast<PxRigidDynamic*>(phys.p_rigid_actor)->isSleeping())
				continue;

			mp_currently_updating_transform = &transform;
			UpdateTransformCompFromGlobalPose(phys.p_rigid_actor->getGlobalPose(), transform);
			mp_currently_updating_transform = nullptr;
		}

		for (auto [entity, controller, transform] : mp_registry->view<CharacterControllerComponent, TransformComponent>().each()) {
			mp_currently_updating_transform = &transform;
			PxExtendedVec3 pos = controller.mp_controller->getPosition();
			transform.SetAbsolutePosition({ pos.x, pos.y, pos.z });
			mp_currently_updating_transform = nullptr;
		}
	}

	void PhysicsSystem::RemoveComponent(CharacterControllerComponent* p_comp) {
		p_comp->mp_controller->release();
	};


	void PhysicsSystem::InitComponent(CharacterControllerComponent* p_comp) {
		PxCapsuleControllerDesc desc;
		desc.height = 2.0;
		desc.radius = 0.5;
		desc.material = m_physics_materials[0];
		desc.stepOffset = 1.8f;
		p_comp->mp_controller = mp_controller_manager->createController(desc);

		auto abs_transforms = p_comp->GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms();
		p_comp->mp_controller->setPosition(PxExtendedVec3(abs_transforms[0].x, abs_transforms[0].y, abs_transforms[0].z));
	}






}