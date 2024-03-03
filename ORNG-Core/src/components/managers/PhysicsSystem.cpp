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
		scene_desc.gravity = PxVec3(0.f, -9.81f, 0.f);
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
		f1.friction = 0.8f;
		f1.material = p_base_material;
		PxVec3 half_extents({ 0.5f, 0.5f, 1.0f });
		PxCookingParams params{ PxTolerancesScale(Physics::GetToleranceScale()) };
		//setPhysXIntegrationParams(vehicle.mBaseParams.axleDescription, &f1, 1, 0.5, vehicle.mPhysXParams);
		vehicle.mPhysXParams.create(vehicle.mBaseParams.axleDescription, PxQueryFilterData(), nullptr, &f1, 1, 0.5, PxTransform({ 0.0, 0.0, 0.5 }), half_extents, PxTransform(PxIdentity));

		bool result = vehicle.initialize(*Physics::GetPhysics(), params, *p_base_material, true);
		vehicle.setUpActor(*mp_phys_scene, PxTransform(PxIdentity), "Test vehicle");
		PxShape* shapes[5];

		vehicle.mPhysXState.physxActor.rigidBody->getShapes(&shapes[0], 5);
		vehicle.mPhysXState.physxActor.rigidBody->setGlobalPose(TransformComponentToPxTransform(*p_comp->GetEntity()->GetComponent<TransformComponent>()));

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


		vehicle.mBaseParams.ackermannParams[0].strength = 1.0f;
		vehicle.mBaseParams.ackermannParams[0].trackWidth = 1.55f;
		vehicle.mBaseParams.ackermannParams[0].wheelBase = 2.86f;
		vehicle.mBaseParams.frame.setToDefault();
		vehicle.mBaseParams.frame.lngAxis = PxVehicleAxes::eNegZ;
		vehicle.mBaseParams.frame.latAxis = PxVehicleAxes::ePosX;
		vehicle.mBaseParams.frame.vrtAxis = PxVehicleAxes::ePosY;

		vehicle.mBaseParams.scale.scale = 1.0;

		vehicle.mBaseParams.brakeResponseParams->maxResponse = 2875.0f;
		vehicle.mBaseParams.brakeResponseParams->wheelResponseMultipliers[0] = 1.0f;
		vehicle.mBaseParams.brakeResponseParams->wheelResponseMultipliers[1] = 1.0f;
		vehicle.mBaseParams.brakeResponseParams->wheelResponseMultipliers[2] = 1.0f;
		vehicle.mBaseParams.brakeResponseParams->wheelResponseMultipliers[3] = 1.0f;

		vehicle.mBaseParams.steerResponseParams.maxResponse = 0.83;

		vehicle.mBaseParams.steerResponseParams.wheelResponseMultipliers[0] = 0.75f;
		vehicle.mBaseParams.steerResponseParams.wheelResponseMultipliers[1] = 0.75f;
		vehicle.mBaseParams.steerResponseParams.wheelResponseMultipliers[2] = 0.0f;
		vehicle.mBaseParams.steerResponseParams.wheelResponseMultipliers[3] = 0.0f;

		vehicle.mBaseParams.suspensionParams[0].suspensionAttachment.p = { -0.7952629923820496f, -0.10795199871063233f, 1.269219994544983f };
		vehicle.mBaseParams.suspensionParams[1].suspensionAttachment.p = { 0.7952629923820496f, -0.10795199871063233f, 1.269219994544983f };
		vehicle.mBaseParams.suspensionParams[2].suspensionAttachment.p = { -0.7952629923820496f, -0.10795199871063233f, -1.593999981880188f };
		vehicle.mBaseParams.suspensionParams[3].suspensionAttachment.p = { 0.7952629923820496f, -0.10795199871063233f, -1.593999981880188f };

		vehicle.mBaseParams.suspensionForceParams[0].damping = 8528.f;
		vehicle.mBaseParams.suspensionForceParams[0].stiffness = 32833.f;
		vehicle.mBaseParams.suspensionForceParams[0].sprungMass = 553.f;

		vehicle.mBaseParams.suspensionForceParams[1].damping = 8742.f;
		vehicle.mBaseParams.suspensionForceParams[1].stiffness = 33657.f;
		vehicle.mBaseParams.suspensionForceParams[1].sprungMass = 567.f;

		vehicle.mBaseParams.suspensionForceParams[2].damping = 6765.f;
		vehicle.mBaseParams.suspensionForceParams[2].stiffness = 26049.f;
		vehicle.mBaseParams.suspensionForceParams[2].sprungMass = 439.f;

		vehicle.mBaseParams.suspensionForceParams[3].damping = 6985.f;
		vehicle.mBaseParams.suspensionForceParams[3].stiffness = 26894.f;
		vehicle.mBaseParams.suspensionForceParams[3].sprungMass = 453.f;


		vehicle.mBaseParams.suspensionStateCalculationParams.suspensionJounceCalculationType = PxVehicleSuspensionJounceCalculationType::eSWEEP;
		for (int i = 0; i < 4; i++) {
			vehicle.mBaseParams.suspensionComplianceParams[i].suspForceAppPoint.addPair(0, { 0.f, 0.f, -0.11204999685287476f });
			vehicle.mBaseParams.suspensionComplianceParams[i].wheelCamberAngle.addPair(0, 0.f);
			vehicle.mBaseParams.suspensionComplianceParams[i].wheelToeAngle.addPair(0, 0.f);
			vehicle.mBaseParams.suspensionComplianceParams[i].tireForceAppPoint.addPair(0, { 0.f, 0.f, -0.11204999685287476f });

			vehicle.mBaseParams.tireForceParams[i].frictionVsSlip[0][0] = 0.f;
			vehicle.mBaseParams.tireForceParams[i].frictionVsSlip[0][1] = 1.f;
			vehicle.mBaseParams.tireForceParams[i].frictionVsSlip[1][0] = 0.10000000149011612f;
			vehicle.mBaseParams.tireForceParams[i].frictionVsSlip[1][1] = 1.f;
			vehicle.mBaseParams.tireForceParams[i].frictionVsSlip[2][0] = 1.f;
			vehicle.mBaseParams.tireForceParams[i].frictionVsSlip[2][1] = 1.f;
			vehicle.mBaseParams.tireForceParams[i].loadFilter[0][0] = 0.f;
			vehicle.mBaseParams.tireForceParams[i].loadFilter[0][1] = 0.23080000281333924f;
			vehicle.mBaseParams.tireForceParams[i].loadFilter[1][0] = 3.0f;
			vehicle.mBaseParams.tireForceParams[i].loadFilter[1][1] = 3.0f;
			vehicle.mBaseParams.tireForceParams[i].camberStiff = 0.f;
			vehicle.mBaseParams.tireForceParams[i].longStiff = 24525.0f;
			vehicle.mBaseParams.tireForceParams[i].latStiffX = 0.009999999776482582f;
			vehicle.mBaseParams.wheelParams[i].halfWidth = 0.15768450498580934f;
			vehicle.mBaseParams.wheelParams[i].radius = 0.532520031929016f;
			vehicle.mBaseParams.wheelParams[i].mass = 20.f;
			vehicle.mBaseParams.wheelParams[i].moi = 1.1716899871826172f;
			vehicle.mBaseParams.wheelParams[i].dampingRate = 0.25f;

			vehicle.mBaseParams.suspensionParams[i].suspensionAttachment.q = { 0.f, 0.f, 0.f, 1.f };
			vehicle.mBaseParams.suspensionParams[i].suspensionTravelDir = { 0.f, -1.f, 0.f };
			vehicle.mBaseParams.suspensionParams[i].suspensionTravelDist = 0.221110999584198f;
			vehicle.mBaseParams.suspensionParams[i].wheelAttachment.p = { 0.f, 0.f, 0.f };
			vehicle.mBaseParams.suspensionParams[i].wheelAttachment.q = { 0.f, 0.f, 0.f, 1.f };
		}

		for (int i = 0; i < 2; i++) {
			vehicle.mBaseParams.tireForceParams[i].latStiffY = 118699.637252138f;
			vehicle.mBaseParams.tireForceParams[i].restLoad = 5628.72314453125f;
		}

		for (int i = 2; i < 4; i++) {
			vehicle.mBaseParams.tireForceParams[i].latStiffY = 143930.84033118f;
			vehicle.mBaseParams.tireForceParams[i].restLoad = 4604.3134765625f;
		}

		/*
		void create
		(const PxVehicleAxleDescription& axleDescription,
			const PxQueryFilterData& queryFilterData, PxQueryFilterCallback* queryFilterCallback,
			PxVehiclePhysXMaterialFriction* materialFrictions, const PxU32 nbMaterialFrictions, const PxReal defaultFriction,
			const PxTransform& physXActorCMassLocalPose,
			const PxVec3& physXActorBoxShapeHalfExtents, const PxTransform& physxActorBoxShapeLocalPose);*/


		vehicle.mBaseParams.suspensionParams[0].suspensionAttachment.p = { -0.7952629923820496f, -0.10795199871063233f, 1.269219994544983f };
		vehicle.mBaseParams.suspensionParams[1].suspensionAttachment.p = { 0.7952629923820496f, -0.10795199871063233f, 1.269219994544983f };
		vehicle.mBaseParams.suspensionParams[2].suspensionAttachment.p = { -0.7952629923820496f, -0.10795199871063233f, -1.593999981880188f };
		vehicle.mBaseParams.suspensionParams[3].suspensionAttachment.p = { 0.7952629923820496f, -0.10795199871063233f, -1.593999981880188f };
		vehicle.mTransmissionCommandState.gear = vehicle.mTransmissionCommandState.eFORWARD;

		vehicle.mDirectDriveParams.directDriveThrottleResponseParams.maxResponse = 2000.0f;
		vehicle.mDirectDriveParams.directDriveThrottleResponseParams.wheelResponseMultipliers[0] = -1.0f;
		vehicle.mDirectDriveParams.directDriveThrottleResponseParams.wheelResponseMultipliers[1] = -1.0f;
		vehicle.mDirectDriveParams.directDriveThrottleResponseParams.wheelResponseMultipliers[2] = 0.0f;
		vehicle.mDirectDriveParams.directDriveThrottleResponseParams.wheelResponseMultipliers[3] = 0.0f;

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

			if (auto* p_vehicle_comp = p_ent->GetComponent<VehicleComponent>()) {
				p_vehicle_comp->m_vehicle.mPhysXState.physxActor.rigidBody->setGlobalPose(TransformComponentToPxTransform(*p_transform));
			}

			if (auto* p_controller_comp = p_ent->GetComponent<CharacterControllerComponent>()) {
				glm::vec3 pos = p_transform->GetAbsPosition();
				p_controller_comp->mp_controller->setPosition({ pos.x, pos.y, pos.z });
			}

			if (p_phys_comp) {
				if (t_event.sub_event_type == TransformComponent::UpdateType::SCALE || t_event.sub_event_type == TransformComponent::UpdateType::ALL) { // Whole shape needs to be rebuilt
					UpdateComponentState(p_phys_comp);
					return;
				}

				p_phys_comp->p_rigid_actor->setGlobalPose(TransformComponentToPxTransform(*p_transform));
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

		glm::vec3 scale_factor = p_comp->GetEntity()->GetComponent<TransformComponent>()->GetAbsScale();
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

		p_comp->p_shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);

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

		m_entity_lookup[static_cast<const PxActor*>(p_comp->p_rigid_actor)] = std::make_pair(p_comp->GetEntity(), ActorType::RIGID_BODY);
		mp_phys_scene->addActor(*p_comp->p_rigid_actor);
	}





	void PhysicsSystem::InitComponent(PhysicsComponent* p_comp) {
		auto* p_meshc = p_comp->GetEntity()->GetComponent<MeshComponent>();
		auto* p_transform = p_comp->GetEntity()->GetComponent<TransformComponent>();
		auto [pos, scale, rot] = p_transform->GetAbsoluteTransforms();
		glm::vec3 extents = p_transform->GetScale() * (p_meshc ? p_meshc->GetMeshData()->GetAABB().max : glm::vec3(1));

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
		m_entity_lookup[static_cast<const PxActor*>(p_comp->p_rigid_actor)] = std::make_pair(p_comp->GetEntity(), ActorType::RIGID_BODY);
	}




	static void UpdateTransformCompFromGlobalPose(const PxTransform& pose, TransformComponent& transform, PhysicsSystem::ActorType type) {
		PxVec3 phys_pos = pose.p;
		PxQuatT phys_rot = pose.q;

		glm::quat phys_quat = glm::quat(phys_rot.w, phys_rot.x, phys_rot.y, phys_rot.z);
		glm::vec3 orientation = glm::degrees(glm::eulerAngles(phys_quat));


		transform.SetAbsolutePosition(glm::vec3(phys_pos.x, phys_pos.y, phys_pos.z));

		// Only set orientation from normal rigid bodies (character controllers causing bugs)
		if (type == PhysicsSystem::ActorType::RIGID_BODY) [[likely]]
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

			mp_currently_updating_transform = &transform;
			UpdateTransformCompFromGlobalPose(vehicle.m_vehicle.mPhysXState.physxActor.rigidBody->getGlobalPose() * shape[0]->getLocalPose(), transform, ActorType::VEHICLE);
			mp_currently_updating_transform = nullptr;
		}


		ORNG_PROFILE_FUNC();
		m_accumulator -= m_step_size * 1000.f;
		mp_phys_scene->simulate(m_step_size);
		mp_phys_scene->fetchResults(true);
		PxU32 num_active_actors;
		PxActor** active_actors = mp_phys_scene->getActiveActors(num_active_actors);


		for (int i = 0; i < num_active_actors; i++) {
			if (auto ret = TryGetEntityAndTypeFromPxActor(active_actors[i]); ret.has_value()) {
				auto& [p_ent, type] = ret.value();
				auto* p_transform = p_ent->GetComponent<TransformComponent>();
				mp_currently_updating_transform = p_transform;
				UpdateTransformCompFromGlobalPose(static_cast<PxRigidActor*>(active_actors[i])->getGlobalPose(), *p_transform, type);
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
		// Remove from cache
		m_entity_lookup.erase(static_cast<const PxActor*>(p_comp->mp_controller->getActor()));

		p_comp->mp_controller->release();
	};


	void PhysicsSystem::InitComponent(CharacterControllerComponent* p_comp) {
		PxCapsuleControllerDesc desc;
		desc.height = 1.8;
		desc.radius = 0.1;
		desc.material = AssetManager::GetAsset<PhysXMaterialAsset>(ORNG_BASE_PHYSX_MATERIAL_ID)->p_material;
		desc.stepOffset = 1.8f;
		p_comp->mp_controller = mp_controller_manager->createController(desc);

		// Cache entity
		m_entity_lookup[static_cast<const PxActor*>(p_comp->mp_controller->getActor())] = std::make_pair(p_comp->GetEntity(), ActorType::CHARACTER_CONTROLLER);

		p_comp->mp_controller->getActor()->setGlobalPose(TransformComponentToPxTransform(*p_comp->GetEntity()->GetComponent<TransformComponent>()));
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

		if (mp_phys_scene->raycast(ConvertVec3<glm::vec3, PxVec3>(origin), ConvertVec3<glm::vec3, PxVec3>(unit_dir), max_distance, ray_buffer)) {
			PxRigidActor* p_closest_actor = ray_buffer.block.actor;

			ret.p_entity = TryGetEntityFromPxActor(static_cast<const PxActor*>(p_closest_actor));
			if (!ret.p_entity) {
				ORNG_CORE_ERROR("Failed to find entity from raycast results");
				ret.p_entity = nullptr;
				return ret;
			}
			ret.p_phys_comp = ret.p_entity->GetComponent<PhysicsComponent>();

			ret.hit = true;
			ret.hit_pos = ConvertVec3<PxVec3, glm::vec3>(ray_buffer.block.position);
			ret.hit_normal = ConvertVec3<PxVec3, glm::vec3>(ray_buffer.block.normal);
			ret.hit_dist = ray_buffer.block.distance;
		}

		return ret;
	};

	OverlapQueryResults PhysicsSystem::OverlapQuery(PxGeometry& geom, glm::vec3 pos) {
		PxOverlapHit overlap_hits[256];
		PxOverlapBuffer overlap_buffer{ overlap_hits, 256 };
		OverlapQueryResults ret;

		if (mp_phys_scene->overlap(geom, PxTransform(ConvertVec3<glm::vec3, PxVec3>(pos)), overlap_buffer)) {
			for (int i = 0; i < overlap_buffer.getNbAnyHits(); i++) {
				auto* p_ent = TryGetEntityFromPxActor(static_cast<const PxActor*>(overlap_hits[i].actor));

				if (!p_ent) {
					ORNG_CORE_ERROR("Failed to find entity from overlap result");
					continue;
				}

				ret.entities.push_back(p_ent);

			}
		}

		return ret;
	}

}