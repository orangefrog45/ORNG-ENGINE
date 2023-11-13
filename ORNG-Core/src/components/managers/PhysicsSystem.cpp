#include "pch/pch.h"
#include "components/ComponentSystems.h"
#include "physics/Physics.h"
#include "scene/SceneEntity.h"
#include "glm/glm/gtc/quaternion.hpp"
#include "events/EventManager.h"
#include "rendering/MeshAsset.h"
#include "util/Timers.h"


namespace ORNG {
	using namespace physx;
	// Conversion from glm::vec3 to PxVec3


	inline static void OnPhysComponentAdd(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<PhysicsComponent>(registry, entity, Events::ECS_EventType::COMP_ADDED);
	}


	inline static void OnPhysComponentDestroy(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<PhysicsComponent>(registry, entity, Events::ECS_EventType::COMP_DELETED);
	}


	inline static void OnCharacterControllerComponentAdd(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<CharacterControllerComponent>(registry, entity, Events::ECS_EventType::COMP_ADDED);
	}

	inline static void OnCharacterControllerComponentDestroy(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<CharacterControllerComponent>(registry, entity, Events::ECS_EventType::COMP_DELETED);
	}

	inline static void OnFixedJointAdd(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<FixedJointComponent>(registry, entity, Events::ECS_EventType::COMP_ADDED);
	}

	inline static void OnFixedJointDestroy(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<FixedJointComponent>(registry, entity, Events::ECS_EventType::COMP_DELETED);
	}






	PhysicsSystem::PhysicsSystem(entt::registry* p_registry, uint64_t scene_uuid, Scene* p_scene) : mp_registry(p_registry), ComponentSystem(scene_uuid), mp_scene(p_scene) {
	};


	PxFilterFlags FilterShaderExample(
		PxFilterObjectAttributes attributes0, PxFilterData filterData0,
		PxFilterObjectAttributes attributes1, PxFilterData filterData1,
		PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
	{
		// let triggers through
		if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
		{
			pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
			return PxFilterFlag::eDEFAULT;
		}
		// generate contacts for all that were not filtered above
		pairFlags = PxPairFlag::eCONTACT_DEFAULT;

		// trigger the contact callback for pairs (A,B) where
		// the filtermask of A contains the ID of B and vice versa.
		if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
			pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;

		// Flag stays active so events can be passed to the engine
		pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;


		return PxFilterFlag::eDEFAULT;
	}


	void PhysicsSystem::OnLoad() {
		PxBroadPhaseDesc bpDesc(PxBroadPhaseType::eABP);

		mp_broadphase = PxCreateBroadPhase(bpDesc);
		mp_aabb_manager = PxCreateAABBManager(*mp_broadphase);

		PxTolerancesScale scale(1.f);
		PxSceneDesc scene_desc{ scale };
		scene_desc.filterShader = FilterShaderExample;
		scene_desc.gravity = PxVec3(0, -9.81, 0);
		scene_desc.cpuDispatcher = Physics::GetCPUDispatcher();
		scene_desc.cudaContextManager = Physics::GetCudaContextManager();
		scene_desc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
		scene_desc.flags |= PxSceneFlag::eENABLE_PCM;
		scene_desc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
		scene_desc.broadPhaseType = PxBroadPhaseType::eGPU;
		scene_desc.simulationEventCallback = &m_collision_callback;


		mp_phys_scene = Physics::GetPhysics()->createScene(scene_desc);
		mp_phys_scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
		mp_phys_scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LOCAL_FRAMES, 1.0f);
		mp_phys_scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LIMITS, 1.0f);
		mp_controller_manager = PxCreateControllerManager(*mp_phys_scene);
		m_physics_materials.push_back(Physics::GetPhysics()->createMaterial(0.25f, 0.1f, 0.1f));
		m_physics_materials[0]->acquireReference();


		InitListeners();
		mp_registry->on_construct<PhysicsComponent>().connect<&OnPhysComponentAdd>();
		mp_registry->on_destroy<PhysicsComponent>().connect<&OnPhysComponentDestroy>();

		mp_registry->on_construct<FixedJointComponent>().connect<&OnFixedJointAdd>();
		mp_registry->on_destroy<FixedJointComponent>().connect<&OnFixedJointDestroy>();

		mp_registry->on_construct<CharacterControllerComponent>().connect<&OnCharacterControllerComponentAdd>();
		mp_registry->on_destroy<CharacterControllerComponent>().connect<&OnCharacterControllerComponentDestroy>();
	}


	void PhysicsSystem::InitListeners() {
		// Initialize event listeners
		//
	// Physics listener
		m_phys_listener.scene_id = GetSceneUUID();
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
			};
			};

		// Joint listener
		m_joint_listener.scene_id = GetSceneUUID();
		m_joint_listener.OnEvent = [this](const Events::ECS_Event<FixedJointComponent>& t_event) {
			switch (t_event.event_type) {
				using enum Events::ECS_EventType;
			case COMP_ADDED:
				InitComponent(t_event.affected_components[0]);
				break;
			case COMP_UPDATED:
				HandleComponentUpdate(t_event);
				break;
			case COMP_DELETED:
				RemoveComponent(t_event.affected_components[0]);
				break;
			}
			};


		// Character controller listener
		m_character_controller_listener.scene_id = GetSceneUUID();
		m_character_controller_listener.OnEvent = [this](const Events::ECS_Event<CharacterControllerComponent>& t_event) {
			using enum Events::ECS_EventType;
			switch (t_event.event_type) {
			case COMP_ADDED: // Only doing index 0 because these events will only ever affect a single component currently
				InitComponent(t_event.affected_components[0]);
				break;
			case COMP_DELETED:
				RemoveComponent(t_event.affected_components[0]);
				break;
			}
			};


		// Transform update listener
		m_transform_listener.scene_id = GetSceneUUID();
		m_transform_listener.OnEvent = [this](const Events::ECS_Event<TransformComponent>& t_event) {
			OnTransformEvent(t_event);
			};

		Events::EventManager::RegisterListener(m_phys_listener);
		Events::EventManager::RegisterListener(m_joint_listener);
		Events::EventManager::RegisterListener(m_character_controller_listener);
		Events::EventManager::RegisterListener(m_transform_listener);
	}

	void PhysicsSystem::InitComponent(FixedJointComponent* p_comp) {
	}

	void PhysicsSystem::RemoveComponent(FixedJointComponent* p_comp) {
		if (p_comp->mp_joint)
			p_comp->mp_joint->release();
	}

	void PhysicsSystem::HandleComponentUpdate(const Events::ECS_Event<FixedJointComponent>& t_event) {
		switch (t_event.sub_event_type) {
		case JointEventType::CONNECT:
			std::pair<PhysicsComponent*, PhysicsComponent*> comps = std::any_cast<std::pair<PhysicsComponent*, PhysicsComponent*>>(t_event.data_payload);

			auto middle = (comps.first->p_rigid_actor->getGlobalPose().p + comps.second->p_rigid_actor->getGlobalPose().p) * 0.5f;

			PxTransform m0(middle - comps.first->p_rigid_actor->getGlobalPose().p);
			PxTransform m1(middle - comps.second->p_rigid_actor->getGlobalPose().p);

			auto* p_phys = Physics::GetPhysics();
			t_event.affected_components[0]->mp_joint = PxFixedJointCreate(*p_phys, comps.first->p_rigid_actor, m0, comps.second->p_rigid_actor, m1);
			t_event.affected_components[0]->mp_joint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);
			break;
		}
	}



	void PhysicsSystem::RemoveComponent(PhysicsComponent* p_comp) {
		m_entity_lookup.erase(static_cast<const PxActor*>(p_comp->p_rigid_actor));

		mp_phys_scene->removeActor(*p_comp->p_rigid_actor);
		p_comp->p_rigid_actor->release();

		p_comp->p_shape->release();
		p_comp->p_material->release();
	}



	void PhysicsSystem::OnUnload() {
		DeinitListeners();

		for (auto* p_material : m_physics_materials) {
			p_material->release();
		}

		m_physics_materials.clear();

		mp_controller_manager->release();
		mp_aabb_manager->release();
		mp_phys_scene->release();
		PX_RELEASE(mp_broadphase);
	}

	void PhysicsSystem::DeinitListeners() {
		Events::EventManager::DeregisterListener(m_phys_listener.GetRegisterID());
		Events::EventManager::DeregisterListener(m_character_controller_listener.GetRegisterID());
		Events::EventManager::DeregisterListener(m_transform_listener.GetRegisterID());
		Events::EventManager::DeregisterListener(m_joint_listener.GetRegisterID());
	}


	void PhysicsSystem::OnTransformEvent(const Events::ECS_Event<TransformComponent>& t_event) {
		if (t_event.event_type == Events::ECS_EventType::COMP_UPDATED) {
			auto* p_transform = t_event.affected_components[0];


			if (p_transform == mp_currently_updating_transform) // Ignore transform event if it was updated by the physics engine, as the states are already synced
				return;

			// Check for both types of physics component
			auto* p_ent = t_event.affected_components[0]->GetEntity();
			auto* p_phys_comp = p_ent->GetComponent<PhysicsComponent>();

			if (auto* p_controller_comp = p_ent->GetComponent<CharacterControllerComponent>()) {
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
				glm::quat n_quat{ glm::radians(n_transforms[2]) };

				PxVec3 px_pos{ abs_pos.x, abs_pos.y, abs_pos.z };
				PxQuat n_px_quat{ n_quat.x, n_quat.y, n_quat.z, n_quat.w };
				PxTransform px_transform{ px_pos, n_px_quat };

				p_phys_comp->p_rigid_actor->setGlobalPose(px_transform);
			}
		}
	}




	PxTriangleMesh* PhysicsSystem::GetOrCreateTriangleMesh(const MeshAsset* p_mesh_asset) {
		if (m_triangle_meshes.contains(p_mesh_asset))
			return m_triangle_meshes[p_mesh_asset];


		PxTolerancesScale scale(1.f);
		const MeshVAO& vao = p_mesh_asset->GetVAO();
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






	void PhysicsSystem::UpdateComponentState(PhysicsComponent* p_comp) {
		// Update component geometry
		auto* p_mesh_comp = p_comp->GetEntity()->GetComponent<MeshComponent>();
		const AABB& aabb = p_mesh_comp ? p_mesh_comp->GetMeshData()->GetAABB() : AABB(glm::vec3(-1), glm::vec3(1));

		glm::vec3 scale_factor = p_comp->GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[1];
		glm::vec3 scaled_extents = aabb.max * scale_factor;


		switch (p_comp->m_geometry_type) {
		case PhysicsComponent::SPHERE:
			p_comp->p_shape->release();
			p_comp->p_shape = Physics::GetPhysics()->createShape(PxSphereGeometry(glm::max(glm::max(scaled_extents.x, scaled_extents.y), scaled_extents.z)), *p_comp->p_material, p_comp->IsTrigger());
			break;
		case PhysicsComponent::BOX:
			p_comp->p_shape->release();
			p_comp->p_shape = Physics::GetPhysics()->createShape(PxBoxGeometry(scaled_extents.x, scaled_extents.y, scaled_extents.z), *p_comp->p_material, p_comp->IsTrigger());
			break;
		case PhysicsComponent::TRIANGLE_MESH:
			if (!p_mesh_comp)
				return;

			p_comp->p_shape->release();
			PxTriangleMesh* aTriangleMesh = GetOrCreateTriangleMesh(p_mesh_comp->GetMeshData());
			p_comp->p_shape = Physics::GetPhysics()->createShape(PxTriangleMeshGeometry(aTriangleMesh, PxMeshScale(PxVec3(scale_factor.x, scale_factor.y, scale_factor.z))), *p_comp->p_material, p_comp->IsTrigger());
			break;
		}


		if (p_comp->IsTrigger()) {
			p_comp->p_shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, false);
			p_comp->p_shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);

			p_comp->p_shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
			p_comp->p_shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
		}


		p_comp->p_shape->acquireReference();

		// Update rigid body type
		PxTransform current_transform = p_comp->p_rigid_actor->getGlobalPose();
		m_entity_lookup.erase(static_cast<const PxActor*>(p_comp->p_rigid_actor));

		p_comp->p_rigid_actor->release();


		if (p_comp->m_body_type == PhysicsComponent::STATIC) {
			p_comp->p_rigid_actor = PxCreateStatic(*Physics::GetPhysics(), current_transform, *p_comp->p_shape);
		}
		else if (p_comp->m_body_type == PhysicsComponent::DYNAMIC) {
			p_comp->p_rigid_actor = PxCreateDynamic(*Physics::GetPhysics(), current_transform, *p_comp->p_shape, 1.f);
		}


		m_entity_lookup[static_cast<const PxActor*>(p_comp->p_rigid_actor)] = p_comp->GetEntity();
		mp_phys_scene->addActor(*p_comp->p_rigid_actor);
	}





	void PhysicsSystem::InitComponent(PhysicsComponent* p_comp) {
		auto* p_meshc = p_comp->GetEntity()->GetComponent<MeshComponent>();
		auto* p_transform = p_comp->GetEntity()->GetComponent<TransformComponent>();
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

		if (p_comp->m_body_type == PhysicsComponent::STATIC) {
			p_comp->p_rigid_actor = PxCreateStatic(*Physics::GetPhysics(), PxTransform(PxVec3(pos.x, pos.y, pos.z), px_quat), *p_comp->p_shape);
		}
		else if (p_comp->m_body_type == PhysicsComponent::DYNAMIC) {
			p_comp->p_rigid_actor = PxCreateDynamic(*Physics::GetPhysics(), PxTransform(PxVec3(pos.x, pos.y, pos.z), px_quat), *p_comp->p_shape, 1.f);
		}

		mp_phys_scene->addActor(*p_comp->p_rigid_actor);
		p_comp->p_rigid_actor->setActorFlag(PxActorFlag::eVISUALIZATION, true);

		// Store in entity lookup map for fast retrieval
		m_entity_lookup[static_cast<const PxActor*>(p_comp->p_rigid_actor)] = p_comp->GetEntity();
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
		m_accumulator += ts;

		if (m_accumulator < m_step_size)
			return;

		m_accumulator -= m_step_size * 1000.f;
		mp_phys_scene->simulate(m_step_size);
		mp_phys_scene->fetchResults(true);

		PxU32 num_active_actors;
		PxActor** active_actors = mp_phys_scene->getActiveActors(num_active_actors);

		for (int i = 0; i < num_active_actors; i++) {
			if (auto* p_ent = TryGetEntityFromPxActor(active_actors[i])) {
				auto* p_transform = p_ent->GetComponent<TransformComponent>();
				mp_currently_updating_transform = p_transform;
				UpdateTransformCompFromGlobalPose(static_cast<PxRigidActor*>(active_actors[i])->getGlobalPose(), *p_transform);
				mp_currently_updating_transform = nullptr;
			}
		}


		// Process OnCollision callbacks
		for (auto& pair : m_entity_collision_queue) {
			auto* p_first_script = pair.first->GetComponent<ScriptComponent>();
			auto* p_second_script = pair.second->GetComponent<ScriptComponent>();
			try {
				if (p_first_script)
					p_first_script->p_instance->OnCollide(pair.second);

				if (p_second_script)
					p_second_script->p_instance->OnCollide(pair.first);
			}
			catch (std::exception e) {
				ORNG_CORE_ERROR("Script OnCollision err for collision pair '{0}', '{1}' : '{2}'", pair.first->name, pair.second->name, e.what());
			}
		}
#
		for (auto& tuple : m_trigger_event_queue) {
			auto [event_type, p_ent, p_trigger] = tuple;
			auto* p_script = p_trigger->GetComponent<ScriptComponent>();
			if (!p_script)
				continue;

			try {
				if (event_type == ENTERED)
					p_script->p_instance->OnTriggerEnter(p_ent);
				else
					p_script->p_instance->OnTriggerLeave(p_ent);
			}
			catch (std::exception e) {
				ORNG_CORE_ERROR("Script trigger event err for pair '{0}', trigger: '{1}' : '{2}'", p_ent->name, p_trigger->name, e.what());
			}
		}
		unsigned size = m_entity_collision_queue.size();
		unsigned size_trigger = m_entity_collision_queue.size();

		m_entity_collision_queue.clear();
		m_trigger_event_queue.clear();
		m_entity_collision_queue.reserve(size);
		m_trigger_event_queue.reserve(size_trigger);
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




	void PhysicsSystem::PhysCollisionCallback::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
	{
		SceneEntity* p_first_ent = mp_system->TryGetEntityFromPxActor(pairHeader.actors[0]);
		SceneEntity* p_second_ent = mp_system->TryGetEntityFromPxActor(pairHeader.actors[1]);

		if (!p_first_ent || !p_second_ent) {
			ORNG_CORE_ERROR("PhysCollisionCallback failed to find entities from collision event");
			return;
		}

		mp_system->m_entity_collision_queue.push_back(std::make_pair(p_first_ent, p_second_ent));
	}

	void PhysicsSystem::PhysCollisionCallback::onTrigger(PxTriggerPair* pairs, PxU32 count) {
		for (int i = 0; i < count; i++) {
			// ignore pairs when shapes have been deleted
			if (pairs[i].flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
				continue;

			auto* p_ent = mp_system->TryGetEntityFromPxActor(pairs[i].otherActor);
			if (auto* p_trigger = mp_system->TryGetEntityFromPxActor(pairs[i].triggerActor); p_trigger && p_ent) {
				if (auto* p_script = p_trigger->GetComponent<ScriptComponent>()) {
					if (pairs[i].status == PxPairFlag::eNOTIFY_TOUCH_FOUND) {
						// Object entered the trigger
						mp_system->m_trigger_event_queue.push_back(std::make_tuple(ENTERED, p_ent, p_trigger));
					}
					else if (pairs[i].status == PxPairFlag::eNOTIFY_TOUCH_LOST) {
						// Object left the trigger
						mp_system->m_trigger_event_queue.push_back(std::make_tuple(EXITED, p_ent, p_trigger));
					}
				}
			}
		}
	};



	RaycastResults PhysicsSystem::Raycast(glm::vec3 origin, glm::vec3 unit_dir, float max_distance) {
		PxRaycastBuffer ray_buffer;                 // [out] Raycast results
		RaycastResults ret;

		if (bool status = mp_phys_scene->raycast(ToPxVec3(origin), ToPxVec3(unit_dir), max_distance, ray_buffer)) {
			PxRigidActor* p_closest_actor = ray_buffer.block.actor;

			// Check dynamic physics components
			for (auto [entity, phys_comp] : mp_registry->view<PhysicsComponent>().each()) {
				if (phys_comp.p_rigid_actor == p_closest_actor) {
					ret.p_phys_comp = &phys_comp;
					ret.p_entity = phys_comp.GetEntity();
				}
			}

			ret.hit = true;
			ret.hit_pos = ToGlmVec3(ray_buffer.block.position);
			ret.hit_normal = ToGlmVec3(ray_buffer.block.normal);
			ret.hit_dist = ray_buffer.block.distance;
		}

		return ret;
	}
}