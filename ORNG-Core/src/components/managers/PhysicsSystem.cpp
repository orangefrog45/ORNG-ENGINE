#include "pch/pch.h"
#include "components/ComponentSystems.h"
#include "physics/Physics.h"
#include "scene/SceneEntity.h"
#include "glm/glm/gtc/quaternion.hpp"
#include "events/EventManager.h"
#include "rendering/MeshAsset.h"


namespace ORNG {
	using namespace physx;
	// Conversion from glm::vec3 to PxVec3
	inline PxVec3 ToPxVec3(const glm::vec3& glmVec) {
		return PxVec3(glmVec.x, glmVec.y, glmVec.z);
	}

	// Conversion from PxVec3 to glm::vec3
	inline glm::vec3 ToGlmVec3(const PxVec3& pxVec) {
		return glm::vec3(pxVec.x, pxVec.y, pxVec.z);
	}


	RaycastResults PhysicsSystem::Raycast(glm::vec3 origin, glm::vec3 unit_dir, float max_distance) {
		PxRaycastBuffer ray_buffer;                 // [out] Raycast results
		RaycastResults ret;

		if (bool status = mp_scene->raycast(ToPxVec3(origin), ToPxVec3(unit_dir), max_distance, ray_buffer)) {
			PxRigidActor* p_closest_actor = ray_buffer.block.actor;

			// Check dynamic physics components
			for (auto [entity, phys_comp] : mp_registry->view<PhysicsComponentDynamic>().each()) {
				if (phys_comp.p_rigid_actor == p_closest_actor) {
					ret.p_phys_comp = &phys_comp;
					ret.p_entity = phys_comp.GetEntity();
				}
			}

			// Check static physics components
			if (ret.p_phys_comp == nullptr) {
				for (auto [entity, phys_comp] : mp_registry->view<PhysicsComponentStatic>().each()) {
					if (phys_comp.p_rigid_actor == p_closest_actor) {
						ret.p_phys_comp = &phys_comp;
						ret.p_entity = phys_comp.GetEntity();
					}
				}
			}

			ret.hit = true;
			ret.hit_pos = ToGlmVec3(ray_buffer.block.position);
			ret.hit_normal = ToGlmVec3(ray_buffer.block.normal);
			ret.hit_dist = ray_buffer.block.distance;
		}

		return ret;
	}

	template <std::derived_from<PhysicsCompBase> T>
	static void OnPhysComponentAdd(entt::registry& registry, entt::entity entity) {
		Events::ECS_Event<PhysicsCompBase> e_event;
		e_event.affected_components.push_back(static_cast<PhysicsCompBase*>(&registry.get<T>(entity)));
		e_event.event_type = Events::ECS_EventType::COMP_ADDED;
		Events::EventManager::DispatchEvent(e_event);
	}

	template <std::derived_from<PhysicsCompBase> T>
	static void OnPhysComponentDestroy(entt::registry& registry, entt::entity entity) {
		Events::ECS_Event<PhysicsCompBase> e_event;
		e_event.affected_components.push_back(static_cast<PhysicsCompBase*>(&registry.get<T>(entity)));
		e_event.event_type = Events::ECS_EventType::COMP_DELETED;
		Events::EventManager::DispatchEvent(e_event);
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
		m_phys_listener.OnEvent = [this](const Events::ECS_Event<PhysicsCompBase>& t_event) {
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


				if (p_transform == mp_currently_updating_transform) // Ignore transform event if it was updated by the physics engine, as the states are already synced
					return;

				// Check for both types of physics component
				auto* p_phys_comp = static_cast<PhysicsCompBase*>(p_transform->GetEntity()->GetComponent<PhysicsComponentDynamic>());
				if (!p_phys_comp)
					p_phys_comp = static_cast<PhysicsCompBase*>(p_transform->GetEntity()->GetComponent<PhysicsComponentStatic>());

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

		PxTolerancesScale scale(1.f);

		PxSceneDesc scene_desc{ scale };
		scene_desc.filterShader = PxDefaultSimulationFilterShader;
		scene_desc.gravity = PxVec3(0, -9.81, 0);
		scene_desc.cpuDispatcher = Physics::GetCPUDispatcher();
		scene_desc.cudaContextManager = Physics::GetCudaContextManager();
		scene_desc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
		scene_desc.flags |= PxSceneFlag::eENABLE_PCM;
		scene_desc.broadPhaseType = PxBroadPhaseType::eGPU;


		mp_scene = Physics::GetPhysics()->createScene(scene_desc);
		mp_controller_manager = PxCreateControllerManager(*mp_scene);
		m_physics_materials.push_back(Physics::GetPhysics()->createMaterial(0.25f, 0.1f, 0.1f));
		m_physics_materials[0]->acquireReference();

		mp_registry->on_construct<PhysicsComponentDynamic>().connect<&OnPhysComponentAdd<PhysicsComponentDynamic>>();
		mp_registry->on_construct<PhysicsComponentStatic>().connect<&OnPhysComponentAdd<PhysicsComponentStatic>>();
		mp_registry->on_destroy<PhysicsComponentDynamic>().connect<&OnPhysComponentDestroy<PhysicsComponentDynamic>>();
		mp_registry->on_destroy<PhysicsComponentStatic>().connect<&OnPhysComponentDestroy<PhysicsComponentStatic>>();

		mp_registry->on_construct<CharacterControllerComponent>().connect<&OnCharacterControllerComponentAdd>();
		mp_registry->on_destroy<CharacterControllerComponent>().connect<&OnCharacterControllerComponentDestroy>();

	}


	void PhysicsSystem::RemoveComponent(PhysicsCompBase* p_comp) {
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


		PxTolerancesScale scale(1.f);
		const VAO& vao = p_mesh_asset->GetVAO();
		PxCookingParams params(scale);
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
		meshDesc.points.count = vao.vertex_data.positions.size() / 3;
		meshDesc.points.stride = sizeof(float) * 3;
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



	void PhysicsSystem::UpdateComponentState(PhysicsCompBase* p_comp) {
		// Update component geometry
		const auto* p_mesh_comp = p_comp->GetEntity()->GetComponent<MeshComponent>();
		const AABB& aabb = p_mesh_comp ? p_mesh_comp->GetMeshData()->GetAABB() : AABB(glm::vec3(-1), glm::vec3(1));

		glm::vec3 scale_factor = p_comp->GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[1];
		glm::vec3 scaled_extents = aabb.max * scale_factor;


		switch (p_comp->geometry_type) {
		case PhysicsCompBase::SPHERE:
			p_comp->p_shape->release();
			p_comp->p_shape = Physics::GetPhysics()->createShape(PxSphereGeometry(glm::max(glm::max(scaled_extents.x, scaled_extents.y), scaled_extents.z)), *p_comp->p_material);
			break;
		case PhysicsCompBase::BOX:
			p_comp->p_shape->release();
			p_comp->p_shape = Physics::GetPhysics()->createShape(PxBoxGeometry(scaled_extents.x, scaled_extents.y, scaled_extents.z), *p_comp->p_material);
			break;
		case PhysicsCompBase::TRIANGLE_MESH:
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


		if (dynamic_cast<PhysicsComponentStatic*>(p_comp)) {
			p_comp->p_rigid_actor = PxCreateStatic(*Physics::GetPhysics(), current_transform, *p_comp->p_shape);
		}
		else if (dynamic_cast<PhysicsComponentDynamic*>(p_comp)) {
			p_comp->p_rigid_actor = PxCreateDynamic(*Physics::GetPhysics(), current_transform, *p_comp->p_shape, 1.f);
		}

		mp_scene->addActor(*p_comp->p_rigid_actor);
	}


	void PhysicsSystem::InitComponent(PhysicsCompBase* p_comp) {
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

		if (dynamic_cast<PhysicsComponentStatic*>(p_comp)) {
			p_comp->p_rigid_actor = PxCreateStatic(*Physics::GetPhysics(), PxTransform(PxVec3(pos.x, pos.y, pos.z), px_quat), *p_comp->p_shape);
		}
		else if (dynamic_cast<PhysicsComponentDynamic*>(p_comp)) {
			p_comp->p_rigid_actor = PxCreateDynamic(*Physics::GetPhysics(), PxTransform(PxVec3(pos.x, pos.y, pos.z), px_quat), *p_comp->p_shape, 1.f);
		}

		mp_scene->addActor(*p_comp->p_rigid_actor);

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

		m_accumulator -= m_step_size * 1000.f;
		mp_scene->simulate(m_step_size);
		mp_scene->fetchResults(true);

		for (auto [entity, phys, transform] : mp_registry->view<PhysicsComponentDynamic, TransformComponent>().each()) {
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