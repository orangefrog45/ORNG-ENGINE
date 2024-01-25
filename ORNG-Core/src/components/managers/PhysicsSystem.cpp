#include "pch/pch.h"
#include "components/ComponentSystems.h"
#include "physics/Physics.h"
#include "scene/SceneEntity.h"
#include "glm/glm/gtc/quaternion.hpp"
#include "events/EventManager.h"
#include "rendering/MeshAsset.h"
#include "util/Timers.h"
#include "physx/vehicle2/PxVehicleAPI.h"
#include "physics/vehicles/DirectDrive.h"
#include "assets/AssetManager.h"
#include "physx/extensions/PxParticleExt.h"


namespace ORNG {
	using namespace physx;
	using namespace physx::vehicle2;
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

	inline static void OnVehicleComponentAdd(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<VehicleComponent>(registry, entity, Events::ECS_EventType::COMP_ADDED);
	}

	inline static void OnVehicleComponentDestroy(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<VehicleComponent>(registry, entity, Events::ECS_EventType::COMP_DELETED);
	}



	PhysicsSystem::PhysicsSystem(entt::registry* p_registry, uint64_t scene_uuid, Scene* p_scene) : ComponentSystem(p_registry, scene_uuid), mp_scene(p_scene) {
	};

	PxFilterFlags FilterShader(
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
			pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;

		return PxFilterFlag::eDEFAULT;
	}

// TODO: add in-editor option for this
#define GPU_PHYSICS_ENABLED false

	void PhysicsSystem::OnLoad() {
		PxBroadPhaseDesc bpDesc(PxBroadPhaseType::eABP);

		mp_broadphase = PxCreateBroadPhase(bpDesc);
		mp_aabb_manager = PxCreateAABBManager(*mp_broadphase);

		PxTolerancesScale scale(1.f);
		PxSceneDesc scene_desc{ scale };
		scene_desc.filterShader = FilterShader;
		scene_desc.gravity = PxVec3(0, -9.81, 0);
		scene_desc.cpuDispatcher = Physics::GetCPUDispatcher();
		scene_desc.cudaContextManager = Physics::GetCudaContextManager();
		scene_desc.flags |= PxSceneFlag::eENABLE_PCM;
		scene_desc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
		scene_desc.solverType = PxSolverType::eTGS;


		if constexpr (GPU_PHYSICS_ENABLED) {
			scene_desc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
			scene_desc.broadPhaseType = PxBroadPhaseType::eGPU;
		}
		else {
			scene_desc.broadPhaseType = PxBroadPhaseType::ePABP;
		}

		scene_desc.simulationEventCallback = &m_collision_callback;


		mp_phys_scene = Physics::GetPhysics()->createScene(scene_desc);
		mp_phys_scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
		mp_phys_scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LOCAL_FRAMES, 1.0f);
		mp_phys_scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LIMITS, 1.0f);
		mp_controller_manager = PxCreateControllerManager(*mp_phys_scene);

		InitListeners();


		mp_registry->on_construct<PhysicsComponent>().connect<&OnPhysComponentAdd>();
		mp_registry->on_destroy<PhysicsComponent>().connect<&OnPhysComponentDestroy>();

		mp_registry->on_construct<FixedJointComponent>().connect<&OnFixedJointAdd>();
		mp_registry->on_destroy<FixedJointComponent>().connect<&OnFixedJointDestroy>();

		mp_registry->on_construct<CharacterControllerComponent>().connect<&OnCharacterControllerComponentAdd>();
		mp_registry->on_destroy<CharacterControllerComponent>().connect<&OnCharacterControllerComponentDestroy>();

		mp_registry->on_destroy<VehicleComponent>().connect<&OnVehicleComponentDestroy>();
		mp_registry->on_construct<VehicleComponent>().connect<&OnVehicleComponentAdd>();


		InitVehicleSimulationContext();
	}

	void PhysicsSystem::InitVehicleSimulationContext() {
		m_vehicle_context.setToDefault();
		m_vehicle_context.gravity = mp_phys_scene->getGravity();
		PxVehicleFrame f;
		f.setToDefault();
		f.lngAxis = PxVehicleAxes::eNegZ;
		f.latAxis = PxVehicleAxes::ePosX;
		f.vrtAxis = PxVehicleAxes::ePosY;
		m_vehicle_context.frame = f;
		m_vehicle_context.scale.scale = 1.0;
		m_vehicle_context.physxScene = mp_phys_scene;
		m_vehicle_context.tireStickyParams.stickyParams[PxVehicleTireDirectionModes::eLONGITUDINAL].thresholdSpeed = 0.0f;
		m_vehicle_context.tireStickyParams.stickyParams[PxVehicleTireDirectionModes::eLATERAL].thresholdSpeed = 0.0f;
		m_vehicle_context.physxActorUpdateMode = PxVehiclePhysXActorUpdateMode::eAPPLY_ACCELERATION;
		mp_sweep_mesh = PxVehicleUnitCylinderSweepMeshCreate(m_vehicle_context.frame, *Physics::GetPhysics(), PxCookingParams(PxTolerancesScale(Physics::GetToleranceScale())));
		m_vehicle_context.physxUnitCylinderSweepMesh = mp_sweep_mesh;
	}

	void PhysicsSystem::InitListeners() {
		// Initialize event listeners

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
		/*m_joint_listener.scene_id = GetSceneUUID();
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
			};*/

		m_vehicle_listener.scene_id = GetSceneUUID();
		m_vehicle_listener.OnEvent = [this](const Events::ECS_Event<VehicleComponent>& t_event) {
			switch (t_event.event_type) {
				using enum Events::ECS_EventType;
			case COMP_ADDED:
				InitComponent(t_event.affected_components[0]);
				break;
			case COMP_UPDATED:
				//HandleComponentUpdate(t_event);
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
		//Events::EventManager::RegisterListener(m_joint_listener);
		Events::EventManager::RegisterListener(m_character_controller_listener);
		Events::EventManager::RegisterListener(m_transform_listener);
		Events::EventManager::RegisterListener(m_vehicle_listener);
	}

	bool PhysicsSystem::InitVehicle(VehicleComponent* p_comp) {
		auto& vehicle = p_comp->m_vehicle;

		if (vehicle.mPhysXState.physxActor.rigidBody)
			vehicle.destroy();

		auto* p_base_material = AssetManager::GetAsset<PhysXMaterialAsset>(ORNG_BASE_PHYSX_MATERIAL_ID)->p_material;

		static PxVehiclePhysXMaterialFriction f1;
		f1.friction = 0.8;
		f1.material = p_base_material;
		PxVec3 half_extents({ 0.5, 0.5, 1.0 });
		PxCookingParams params{ PxTolerancesScale(Physics::GetToleranceScale()) };
		//setPhysXIntegrationParams(vehicle.mBaseParams.axleDescription, &f1, 1, 0.5, vehicle.mPhysXParams);
		vehicle.mPhysXParams.create(vehicle.mBaseParams.axleDescription, PxQueryFilterData(), nullptr, &f1, 1, 0.5, PxTransform({ 0.0, 0.0, 0.5 }), half_extents, PxTransform(PxIdentity));

		bool result = vehicle.initialize(*Physics::GetPhysics(), params, *p_base_material, true);
		vehicle.setUpActor(*mp_phys_scene, PxTransform(PxIdentity), "Test vehicle");
		PxShape* shapes[5];
		vehicle.mPhysXState.physxActor.rigidBody->getShapes(&shapes[0], 5);

		for (int i = 0; i < 1; i++) {
			// TODO: Have actual parameters for changing shapes and debug visuals for it
			shapes[i]->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
		}
		vehicle.step(FLT_MIN, m_vehicle_context);

		return result;
	}

	

	void PhysicsSystem::InitComponent(VehicleComponent* p_comp) {
		auto& vehicle = p_comp->m_vehicle;

		auto* p_asset = AssetManager::GetAsset<MeshAsset>(ORNG_BASE_MESH_ID);
		p_comp->p_body_mesh = p_asset;
		p_comp->p_wheel_mesh = p_asset;
		p_comp->m_wheel_materials.push_back(AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID));
		p_comp->m_body_materials.push_back(AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID));

		vehicle.mBaseParams.rigidBodyParams.mass = 2000.0;
		vehicle.mBaseParams.rigidBodyParams.moi = PxVec3(3200.0,
			3414.0,
			750.0);

		PxU32 ids[4] = { 0, 1, 2, 3 };

		vehicle.mBaseParams.axleDescription.setToDefault();
		vehicle.mBaseParams.axleDescription.addAxle(2, &ids[0]);
		vehicle.mBaseParams.axleDescription.addAxle(2, &ids[2]);
		vehicle.mBaseParams.axleDescription.nbWheels = 4;


		vehicle.mBaseParams.ackermannParams[0].strength = 1.0;
		vehicle.mBaseParams.ackermannParams[0].trackWidth = 1.55;
		vehicle.mBaseParams.ackermannParams[0].wheelBase = 2.86;
		vehicle.mBaseParams.frame.setToDefault();
		vehicle.mBaseParams.frame.lngAxis = PxVehicleAxes::eNegZ;
		vehicle.mBaseParams.frame.latAxis = PxVehicleAxes::ePosX;
		vehicle.mBaseParams.frame.vrtAxis = PxVehicleAxes::ePosY;

		vehicle.mBaseParams.scale.scale = 1.0;

		vehicle.mBaseParams.brakeResponseParams->maxResponse = 2875.0;
		vehicle.mBaseParams.brakeResponseParams->wheelResponseMultipliers[0] = 1.0;
		vehicle.mBaseParams.brakeResponseParams->wheelResponseMultipliers[1] = 1.0;
		vehicle.mBaseParams.brakeResponseParams->wheelResponseMultipliers[2] = 1.0;
		vehicle.mBaseParams.brakeResponseParams->wheelResponseMultipliers[3] = 1.0;

		vehicle.mBaseParams.steerResponseParams.maxResponse = 0.83;

		vehicle.mBaseParams.steerResponseParams.wheelResponseMultipliers[0] = 0.75;
		vehicle.mBaseParams.steerResponseParams.wheelResponseMultipliers[1] = 0.75;
		vehicle.mBaseParams.steerResponseParams.wheelResponseMultipliers[2] = 0.0;
		vehicle.mBaseParams.steerResponseParams.wheelResponseMultipliers[3] = 0.0;

		vehicle.mBaseParams.suspensionParams[0].suspensionAttachment.p = { -0.7952629923820496, -0.10795199871063233, 1.269219994544983 };
		vehicle.mBaseParams.suspensionParams[1].suspensionAttachment.p = { 0.7952629923820496, -0.10795199871063233, 1.269219994544983 };
		vehicle.mBaseParams.suspensionParams[2].suspensionAttachment.p = { -0.7952629923820496, -0.10795199871063233, -1.593999981880188 };
		vehicle.mBaseParams.suspensionParams[3].suspensionAttachment.p = { 0.7952629923820496, -0.10795199871063233, -1.593999981880188 };

		vehicle.mBaseParams.suspensionForceParams[0].damping = 8528;
		vehicle.mBaseParams.suspensionForceParams[0].stiffness = 32833;
		vehicle.mBaseParams.suspensionForceParams[0].sprungMass = 553;

		vehicle.mBaseParams.suspensionForceParams[1].damping = 8742;
		vehicle.mBaseParams.suspensionForceParams[1].stiffness = 33657;
		vehicle.mBaseParams.suspensionForceParams[1].sprungMass = 567;

		vehicle.mBaseParams.suspensionForceParams[2].damping = 6765;
		vehicle.mBaseParams.suspensionForceParams[2].stiffness = 26049;
		vehicle.mBaseParams.suspensionForceParams[2].sprungMass = 439;

		vehicle.mBaseParams.suspensionForceParams[3].damping = 6985;
		vehicle.mBaseParams.suspensionForceParams[3].stiffness = 26894;
		vehicle.mBaseParams.suspensionForceParams[3].sprungMass = 453;


		vehicle.mBaseParams.suspensionStateCalculationParams.suspensionJounceCalculationType = PxVehicleSuspensionJounceCalculationType::eSWEEP;
		for (int i = 0; i < 4; i++) {
			vehicle.mBaseParams.suspensionComplianceParams[i].suspForceAppPoint.addPair(0, { 0, 0, -0.11204999685287476 });
			vehicle.mBaseParams.suspensionComplianceParams[i].wheelCamberAngle.addPair(0, 0);
			vehicle.mBaseParams.suspensionComplianceParams[i].wheelToeAngle.addPair(0, 0);
			vehicle.mBaseParams.suspensionComplianceParams[i].tireForceAppPoint.addPair(0, { 0, 0, -0.11204999685287476 });

			vehicle.mBaseParams.tireForceParams[i].frictionVsSlip[0][0] = 0;
			vehicle.mBaseParams.tireForceParams[i].frictionVsSlip[0][1] = 1;
			vehicle.mBaseParams.tireForceParams[i].frictionVsSlip[1][0] = 0.10000000149011612;
			vehicle.mBaseParams.tireForceParams[i].frictionVsSlip[1][1] = 1;
			vehicle.mBaseParams.tireForceParams[i].frictionVsSlip[2][0] = 1;
			vehicle.mBaseParams.tireForceParams[i].frictionVsSlip[2][1] = 1;
			vehicle.mBaseParams.tireForceParams[i].loadFilter[0][0] = 0;
			vehicle.mBaseParams.tireForceParams[i].loadFilter[0][1] = 0.23080000281333924;
			vehicle.mBaseParams.tireForceParams[i].loadFilter[1][0] = 3.0;
			vehicle.mBaseParams.tireForceParams[i].loadFilter[1][1] = 3.0;
			vehicle.mBaseParams.tireForceParams[i].camberStiff = 0;
			vehicle.mBaseParams.tireForceParams[i].longStiff = 24525.0;
			vehicle.mBaseParams.tireForceParams[i].latStiffX = 0.009999999776482582;
			vehicle.mBaseParams.wheelParams[i].halfWidth = 0.15768450498580934;
			vehicle.mBaseParams.wheelParams[i].radius = 0.532520031929016;
			vehicle.mBaseParams.wheelParams[i].mass = 20;
			vehicle.mBaseParams.wheelParams[i].moi = 1.1716899871826172;
			vehicle.mBaseParams.wheelParams[i].dampingRate = 0.25;

			vehicle.mBaseParams.suspensionParams[i].suspensionAttachment.q = { 0, 0, 0, 1 };
			vehicle.mBaseParams.suspensionParams[i].suspensionTravelDir = { 0, -1, 0 };
			vehicle.mBaseParams.suspensionParams[i].suspensionTravelDist = 0.221110999584198;
			vehicle.mBaseParams.suspensionParams[i].wheelAttachment.p = { 0, 0, 0 };
			vehicle.mBaseParams.suspensionParams[i].wheelAttachment.q = { 0, 0, 0, 1 };
		}

		for (int i = 0; i < 2; i++) {
			vehicle.mBaseParams.tireForceParams[i].latStiffY = 118699.637252138;
			vehicle.mBaseParams.tireForceParams[i].restLoad = 5628.72314453125;
		}

		for (int i = 2; i < 4; i++) {
			vehicle.mBaseParams.tireForceParams[i].latStiffY = 143930.84033118;
			vehicle.mBaseParams.tireForceParams[i].restLoad = 4604.3134765625;
		}

		/*
		void create
		(const PxVehicleAxleDescription& axleDescription,
			const PxQueryFilterData& queryFilterData, PxQueryFilterCallback* queryFilterCallback,
			PxVehiclePhysXMaterialFriction* materialFrictions, const PxU32 nbMaterialFrictions, const PxReal defaultFriction,
			const PxTransform& physXActorCMassLocalPose,
			const PxVec3& physXActorBoxShapeHalfExtents, const PxTransform& physxActorBoxShapeLocalPose);*/


		vehicle.mBaseParams.suspensionParams[0].suspensionAttachment.p = { -0.7952629923820496, -0.10795199871063233, 1.269219994544983 };
		vehicle.mBaseParams.suspensionParams[1].suspensionAttachment.p = { 0.7952629923820496, -0.10795199871063233, 1.269219994544983 };
		vehicle.mBaseParams.suspensionParams[2].suspensionAttachment.p = { -0.7952629923820496, -0.10795199871063233, -1.593999981880188 };
		vehicle.mBaseParams.suspensionParams[3].suspensionAttachment.p = { 0.7952629923820496, -0.10795199871063233, -1.593999981880188 };
		vehicle.mTransmissionCommandState.gear = vehicle.mTransmissionCommandState.eFORWARD;



		vehicle.mDirectDriveParams.directDriveThrottleResponseParams.maxResponse = 2000.0;
		vehicle.mDirectDriveParams.directDriveThrottleResponseParams.wheelResponseMultipliers[0] = -1.0;
		vehicle.mDirectDriveParams.directDriveThrottleResponseParams.wheelResponseMultipliers[1] = -1.0;
		vehicle.mDirectDriveParams.directDriveThrottleResponseParams.wheelResponseMultipliers[2] = 0.0;
		vehicle.mDirectDriveParams.directDriveThrottleResponseParams.wheelResponseMultipliers[3] = 0.0;

		InitVehicle(p_comp);
	}



	void PhysicsSystem::RemoveComponent(FixedJointComponent* p_comp) {
		if (p_comp->mp_joint)
			p_comp->mp_joint->release();
	}

	void PhysicsSystem::RemoveComponent(VehicleComponent* p_comp) {
		p_comp->m_vehicle.destroy();
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
		p_comp->p_material->p_material->release();
	}



	void PhysicsSystem::OnUnload() {
		PxVehicleUnitCylinderSweepMeshDestroy(mp_sweep_mesh);
		DeinitListeners();

		mp_controller_manager->release();
		mp_aabb_manager->release();
		mp_phys_scene->release();
		PX_RELEASE(mp_broadphase);
	}

	void PhysicsSystem::DeinitListeners() {
		Events::EventManager::DeregisterListener(m_phys_listener.GetRegisterID());
		Events::EventManager::DeregisterListener(m_character_controller_listener.GetRegisterID());
		Events::EventManager::DeregisterListener(m_transform_listener.GetRegisterID());
		//Events::EventManager::DeregisterListener(m_joint_listener.GetRegisterID());
		Events::EventManager::DeregisterListener(m_vehicle_listener.GetRegisterID());
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
			p_comp->p_shape = Physics::GetPhysics()->createShape(PxSphereGeometry(glm::max(glm::max(scaled_extents.x, scaled_extents.y), scaled_extents.z)), *p_comp->p_material->p_material, p_comp->IsTrigger());
			break;
		case PhysicsComponent::BOX:
			p_comp->p_shape->release();
			p_comp->p_shape = Physics::GetPhysics()->createShape(PxBoxGeometry(scaled_extents.x, scaled_extents.y, scaled_extents.z), *p_comp->p_material->p_material, p_comp->IsTrigger());
			break;
		case PhysicsComponent::TRIANGLE_MESH:
			if (!p_mesh_comp)
				return;

			p_comp->p_shape->release();
			PxTriangleMesh* aTriangleMesh = GetOrCreateTriangleMesh(p_mesh_comp->GetMeshData());
			p_comp->p_shape = Physics::GetPhysics()->createShape(PxTriangleMeshGeometry(aTriangleMesh, PxMeshScale(PxVec3(scale_factor.x, scale_factor.y, scale_factor.z))), *p_comp->p_material->p_material, p_comp->IsTrigger());
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

		p_comp->p_material = AssetManager::GetAsset<PhysXMaterialAsset>(ORNG_BASE_PHYSX_MATERIAL_ID);
		p_comp->p_material->p_material->acquireReference();

		// Setup shape based on mesh AABB if available
		p_comp->p_shape = Physics::GetPhysics()->createShape(PxBoxGeometry(extents.x, extents.y, extents.z), *p_comp->p_material->p_material);
		p_comp->p_shape->acquireReference();

		if (p_comp->m_body_type == PhysicsComponent::STATIC) {
			p_comp->p_rigid_actor = PxCreateStatic(*Physics::GetPhysics(), PxTransform(PxVec3(pos.x, pos.y, pos.z), px_quat), *p_comp->p_shape);
		}
		else if (p_comp->m_body_type == PhysicsComponent::DYNAMIC) {
			p_comp->p_rigid_actor = PxCreateDynamic(*Physics::GetPhysics(), PxTransform(PxVec3(pos.x, pos.y, pos.z), px_quat), *p_comp->p_shape, 1.f);
		}

		mp_phys_scene->addActor(*p_comp->p_rigid_actor);

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


		for (auto [entity, vehicle, transform] : mp_registry->view<VehicleComponent, TransformComponent>().each()) {
			vehicle.m_vehicle.step(m_step_size, m_vehicle_context);
			PxShape* shape[1];
			vehicle.m_vehicle.mPhysXState.physxActor.rigidBody->getShapes(&shape[0], 1);

			UpdateTransformCompFromGlobalPose(vehicle.m_vehicle.mPhysXState.physxActor.rigidBody->getGlobalPose() * shape[0]->getLocalPose(), transform);
		}

		ORNG_PROFILE_FUNC();
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
		desc.material = AssetManager::GetAsset<PhysXMaterialAsset>(ORNG_BASE_PHYSX_MATERIAL_ID)->p_material;
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
	};
}