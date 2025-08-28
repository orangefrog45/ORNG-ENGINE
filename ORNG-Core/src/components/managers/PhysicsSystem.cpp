#include "pch/pch.h"

#include <Jolt/Jolt.h>

#include "components/systems/PhysicsSystem.h"
#include "components/MeshComponent.h"
#include "components/TransformComponent.h"
#include "components/ScriptComponent.h"

#include "assets/AssetManager.h"
#include "events/EventManager.h"
#include "core/FrameTiming.h"
#include "glm/glm/gtc/quaternion.hpp"
#include "glm/glm/gtc/round.hpp"
#include "Jolt/RegisterTypes.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "rendering/MeshAsset.h"
#include "scene/SceneEntity.h"
#include "scene/SerializationUtil.h"
#include "util/Timers.h"
#include "yaml-cpp/yaml.h"

using namespace ORNG;
using namespace JPH;

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

inline static void OnJointAdd(entt::registry& registry, entt::entity entity) {
	ComponentSystem::DispatchComponentEvent<JointComponent>(registry, entity, Events::ECS_EventType::COMP_ADDED);
}

inline static void OnJointDestroy(entt::registry& registry, entt::entity entity) {
	ComponentSystem::DispatchComponentEvent<JointComponent>(registry, entity, Events::ECS_EventType::COMP_DELETED);
}

inline static Vec3 GlmToJph(const glm::vec3& v) {
	return Vec3{v.x, v.y, v.z};
}

inline static Quat GlmToJph(const glm::quat& q) {
	return Quat{q.x, q.y, q.z, q.w};
}

ORNG::PhysicsSystem::PhysicsSystem(Scene* p_scene) : ComponentSystem(p_scene) {
};

void ORNG::PhysicsSystem::OnLoad() {
	s_num_loaded_instances++;

	if (!Factory::sInstance) { // TODO: This probably needs to be set in scripts
		JPH::RegisterDefaultAllocator();
		Factory::sInstance = new Factory{};
		JPH::RegisterTypes();
	}

	mp_temp_allocator = std::make_unique<TempAllocatorImpl>(1024 * 1024 * 10);
	mp_job_system = std::make_unique<JobSystemThreadPool>(cMaxPhysicsJobs, cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

	// Maximum amount of rigid bodies that can be added to the physics system
	constexpr unsigned max_bodies = 10'000;

	// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
	constexpr unsigned num_body_mutexes = 0;

	// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
	// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
	// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
	constexpr unsigned max_body_pairs = max_bodies;

	// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
	// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
	constexpr unsigned max_contact_constraints = max_bodies / 2;

	m_physics_system.Init(max_bodies, num_body_mutexes, max_body_pairs, max_contact_constraints, m_broad_phase_layer_interface,
		m_object_vs_broadphase_layer_filter, m_object_vs_object_layer_filter);
	
	mp_scene->RegisterComponent<PhysicsComponent>();
	mp_scene->RegisterComponent<CharacterControllerComponent>();
	mp_scene->RegisterComponent<JointComponent>();

	InitListeners();

	auto& reg = mp_scene->GetRegistry();
	m_connections[0] = reg.on_construct<PhysicsComponent>().connect<&OnPhysComponentAdd>();
	m_connections[1] = reg.on_destroy<PhysicsComponent>().connect<&OnPhysComponentDestroy>();

	m_connections[2] = reg.on_construct<JointComponent>().connect<&OnJointAdd>();
	m_connections[3] = reg.on_destroy<JointComponent>().connect<&OnJointDestroy>();

	m_connections[4] = reg.on_construct<CharacterControllerComponent>().connect<&OnCharacterControllerComponentAdd>();
	m_connections[5] = reg.on_destroy<CharacterControllerComponent>().connect<&OnCharacterControllerComponentDestroy>();
}

void ORNG::PhysicsSystem::SerializeEntity(SceneEntity& entity, YAML::Emitter* p_emitter) {
	auto& out = *p_emitter;

	if (PhysicsComponent* p_physics_comp = entity.GetComponent<PhysicsComponent>()) {
		out << YAML::Key << "PhysicsComp";
		out << YAML::BeginMap;

		out << YAML::Key << "RigidBodyType" << YAML::Value << p_physics_comp->m_body_type;
		out << YAML::Key << "GeometryType" << YAML::Value << p_physics_comp->m_geometry_type;
		out << YAML::Key << "IsTrigger" << YAML::Value << p_physics_comp->IsTrigger();
		out << YAML::EndMap;
	}
}

void ORNG::PhysicsSystem::DeserializeEntity(SceneEntity& entity, const YAML::Node* p_node) {
	if (auto node = (*p_node)["PhysicsComp"]) {
		auto geometry_type = static_cast<PhysicsComponent::GeometryType>(node["GeometryType"].as<unsigned int>());
		auto body_type = static_cast<PhysicsComponent::RigidBodyType>(node["RigidBodyType"].as<unsigned int>());
		auto is_trigger = node["IsTrigger"].as<bool>();

		entity.AddComponent<PhysicsComponent>(is_trigger, geometry_type, body_type);
	}
}

void ORNG::PhysicsSystem::InitListeners() {
	// Physics listener
	m_phys_listener.scene_id = GetSceneUUID();
	m_phys_listener.OnEvent = [this](const Events::ECS_Event<PhysicsComponent>& t_event) {
		switch (t_event.event_type) {
			using enum Events::ECS_EventType;
		case COMP_ADDED: // Only doing index 0 because these events will only ever affect a single component currently
			InitComponent(t_event.p_component);
			break;
		case COMP_UPDATED:
			UpdateComponentState(t_event.p_component);
			break;
		case COMP_DELETED:
			RemoveComponent(t_event.p_component);
			break;
		}
	};

	// Transform update listener
	m_transform_listener.scene_id = GetSceneUUID();
	m_transform_listener.OnEvent = [this](const Events::ECS_Event<TransformComponent>& t_event) {
		OnTransformEvent(t_event);
	};

	m_serialization_listener.OnEvent = [this](const EntitySerializationEvent& _event) {
		if (_event.event_type == EntitySerializationEvent::Type::SERIALIZING)
			SerializeEntity(*_event.p_entity, _event.data.p_emitter);
		else if (_event.event_type == EntitySerializationEvent::Type::DESERIALIZING)
			DeserializeEntity(*_event.p_entity, _event.data.p_node);
	};

	Events::EventManager::RegisterListener(m_phys_listener);
	Events::EventManager::RegisterListener(m_transform_listener);
	Events::EventManager::RegisterListener(m_serialization_listener);
}

void ORNG::PhysicsSystem::OnUnload() {
	s_num_loaded_instances--;

	if (s_num_loaded_instances == 0) {
		// Nothing is using this factory anymore, so clean it up
		delete Factory::sInstance;
		Factory::sInstance = nullptr;
	}

	for (auto& connection : m_connections) {
		connection.release();
	}

	DeinitListeners();
}

void ORNG::PhysicsSystem::DeinitListeners() {
	Events::EventManager::DeregisterListener(m_phys_listener.GetRegisterID());
	Events::EventManager::DeregisterListener(m_transform_listener.GetRegisterID());
	Events::EventManager::DeregisterListener(m_serialization_listener.GetRegisterID());
}

void ORNG::PhysicsSystem::OnTransformEvent(const Events::ECS_Event<TransformComponent>& t_event) {
	if (t_event.event_type != Events::ECS_EventType::COMP_UPDATED) return;

	auto* p_transform = t_event.p_component;

	if (p_transform == mp_currently_updating_transform) // Ignore transform event if it was updated by the physics engine, as the states are already synced
		return;

	// Check for both types of physics component
	auto* p_ent = t_event.p_component->GetEntity();

	if (auto* p_phys_comp = p_ent->GetComponent<PhysicsComponent>()) {
		if (t_event.sub_event_type == TransformComponent::UpdateType::SCALE || t_event.sub_event_type == TransformComponent::UpdateType::ALL) { // Whole shape needs to be rebuilt
			UpdateComponentState(p_phys_comp);
			return;
		}

		// TODO: Could batch these
		BodyInterface& body_interface = m_physics_system.GetBodyInterface();
		body_interface.SetPositionAndRotation(p_phys_comp->body_id, GlmToJph(p_transform->GetAbsPosition()),
			GlmToJph(p_transform->GetAbsOrientationQuat()), EActivation::Activate);
	}
}

void ORNG::PhysicsSystem::UpdateComponentState(PhysicsComponent* p_comp) {
	ORNG_TRACY_PROFILE;

	auto* p_mesh_comp = p_comp->GetEntity()->GetComponent<MeshComponent>();
	TransformComponent& transform = *p_comp->GetEntity()->GetComponent<TransformComponent>();
	const AABB& aabb = p_mesh_comp ? p_mesh_comp->GetMeshData()->GetAABB() : AABB(glm::vec3(1.f));

	glm::vec3 scale_factor = p_comp->GetEntity()->GetComponent<TransformComponent>()->GetAbsScale();
	glm::vec3 scaled_extents = aabb.extents * scale_factor;

	// The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
	// variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
	BodyInterface& body_interface = m_physics_system.GetBodyInterface(); // TODO: Use non-locking variant

	if (!p_comp->body_id.IsInvalid()) {
		body_interface.RemoveBody(p_comp->body_id);
		body_interface.DestroyBody(p_comp->body_id);
		num_body_events_since_last_broadphase_optimization++;
	}

	EMotionType motion_type = p_comp->m_body_type == PhysicsComponent::STATIC ? EMotionType::Static : EMotionType::Dynamic;
	ObjectLayer layer = p_comp->m_body_type == PhysicsComponent::STATIC ? Layers::NON_MOVING : Layers::MOVING;

	if (p_comp->m_geometry_type == PhysicsComponent::SPHERE) {
		BodyCreationSettings sphere_settings{new SphereShape{glm::max(glm::max(scale_factor.x, scale_factor.y), scale_factor.z)},
			GlmToJph(transform.GetAbsPosition()), GlmToJph(transform.GetAbsOrientationQuat()), motion_type, layer};

		p_comp->body_id = body_interface.CreateAndAddBody(sphere_settings, EActivation::Activate);
	} else if (p_comp->m_geometry_type == PhysicsComponent::BOX) {
		BoxShapeSettings box_shape_settings{Vec3{scaled_extents.x, scaled_extents.y, scaled_extents.z}};
		ShapeSettings::ShapeResult box_shape_result;
		BodyCreationSettings box_settings{new BoxShape{box_shape_settings, box_shape_result},
			GlmToJph(transform.GetAbsPosition()), GlmToJph(transform.GetAbsOrientationQuat()), motion_type, layer};

		p_comp->body_id = body_interface.CreateAndAddBody(box_settings, EActivation::Activate);
	}

	body_interface.SetUserData(p_comp->body_id, reinterpret_cast<size_t>(p_comp->GetEntity()));
	num_body_events_since_last_broadphase_optimization++;

	// Arbitrary threshold
	// OptimizeBroadPhase is expensive so it shouldn't be done very frequently, just whenever a bunch of bodies are added or removed
	if (num_body_events_since_last_broadphase_optimization > 50) {
		m_physics_system.OptimizeBroadPhase();
		num_body_events_since_last_broadphase_optimization = 0;
	}
}


void ORNG::PhysicsSystem::InitComponent(PhysicsComponent* p_comp) {
	UpdateComponentState(p_comp);
}

void ORNG::PhysicsSystem::RemoveComponent(PhysicsComponent* p_comp) {
	if (p_comp->body_id.IsInvalid()) return;

	BodyInterface& body_interface = m_physics_system.GetBodyInterface(); // TODO: Use non-locking variant
	//delete body_interface.GetShape(p_comp->body_id).GetPtr();
	body_interface.RemoveBody(p_comp->body_id);
	body_interface.DestroyBody(p_comp->body_id);

	num_body_events_since_last_broadphase_optimization++;
}


void ORNG::PhysicsSystem::OnUpdate() {
	if (!m_is_updating)
		return;

	ORNG_PROFILE_FUNC();
	float ts = FrameTiming::GetTimeStep();

	m_accumulator += ts * 0.001f;
	if (m_accumulator < step_size) return;

	int num_steps = static_cast<int>(ceil(m_accumulator / step_size));
	float adjusted_step_size = m_accumulator / static_cast<float>(num_steps);
	m_physics_system.Update(adjusted_step_size, num_steps, mp_temp_allocator.get(), mp_job_system.get());

	m_accumulator = 0.f;

	BodyInterface& body_interface = m_physics_system.GetBodyInterface(); // TODO: Use non-locking variant
	BodyIDVector active_bodies;
	m_physics_system.GetActiveBodies(EBodyType::RigidBody, active_bodies);
	for (const auto& body : active_bodies) {
		Quat q;
		Vec3 p;
		body_interface.GetPositionAndRotation(body, p, q);

		auto* p_ent = reinterpret_cast<SceneEntity*>(body_interface.GetUserData(body));
		TransformComponent& transform = *p_ent->GetComponent<TransformComponent>();
		mp_currently_updating_transform = &transform; // Set this so it's ignored by the transform event callbacks while being updated
		transform.SetAbsolutePosition(glm::vec3{p.GetX(), p.GetY(), p.GetZ()});
		transform.SetAbsOrientationQuat(glm::quat{q.GetW(), q.GetX(), q.GetY(), q.GetZ()});
	}

	mp_currently_updating_transform = nullptr;
}

void ORNG::PhysicsSystem::Tick() {
	//auto& reg = mp_scene->GetRegistry();
	//float ts = FrameTiming::GetTimeStep();

	// TODO: Update scene, then update all component transforms and collision callbacks

	// for (int i = 0; i < num_active_actors; i++) {
	// 	SceneEntity* p_ent = static_cast<SceneEntity*>(active_actors[i]->userData);
	// 	auto* p_transform = p_ent->GetComponent<TransformComponent>();
	// 	mp_currently_updating_transform = p_transform;
	// 	UpdateTransformCompFromGlobalPose(static_cast<PxRigidActor*>(active_actors[i])->getGlobalPose(), *p_transform, p_ent->HasComponent<CharacterControllerComponent>() ? ActorType::CHARACTER_CONTROLLER : ActorType::RIGID_BODY);
	// 	mp_currently_updating_transform = nullptr;
	// }
	//
	//
	// // Process OnCollision callbacks
	// for (auto& pair : m_entity_collision_queue) {
	// 	auto* p_first_script = reg.try_get<ScriptComponent>(pair.first);
	// 	auto* p_second_script = reg.try_get<ScriptComponent>(pair.second);
	// 	try {
	// 		if (p_first_script)
	// 			p_first_script->p_instance->OnCollide(reg.get<TransformComponent>(pair.second).GetEntity());
	//
	// 		if (p_second_script)
	// 			p_second_script->p_instance->OnCollide(reg.get<TransformComponent>(pair.first).GetEntity());
	// 	}
	// 	catch (std::exception e) {
	// 		ORNG_CORE_ERROR("Script OnCollision err for collision pair '{0}', '{1}' : '{2}'", p_first_script->GetEntity()->name, p_second_script->GetEntity()->name, e.what());
	// 	}
	// }

	// for (auto& [event_type, ent, trigger] : m_trigger_event_queue) {
	// 	auto* p_script = reg.try_get<ScriptComponent>(trigger);
	// 	if (!p_script)
	// 		continue;
	//
	// 	auto* p_ent = reg.try_get<TransformComponent>(ent)->GetEntity();
	// 	try {
	// 		if (event_type == ENTERED)
	// 			p_script->p_instance->OnTriggerEnter(p_ent);
	// 		else
	// 			p_script->p_instance->OnTriggerLeave(p_ent);
	// 	}
	// 	catch (std::exception e) {
	// 		ORNG_CORE_ERROR("Script trigger event err for pair '{0}', trigger: '{1}' : '{2}'", p_ent->name, p_script->GetEntity()->name, e.what());
	// 	}
	// }
	//
	// for (auto* p_joint : m_joints_to_break) {
	// 	BreakJoint(p_joint);
	// }
	//
	// unsigned size = m_entity_collision_queue.size();
	// unsigned size_trigger = m_entity_collision_queue.size();
	// unsigned size_broken_joints = m_joints_to_break.size();
	//
	// m_entity_collision_queue.clear();
	// m_trigger_event_queue.clear();
	// m_joints_to_break.clear();
	//
	// m_entity_collision_queue.reserve(size);
	// m_trigger_event_queue.reserve(size_trigger);
	// m_joints_to_break.reserve(size_broken_joints);
}
