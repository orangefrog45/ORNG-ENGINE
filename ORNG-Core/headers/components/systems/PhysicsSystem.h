#pragma once

#include "components/systems/ComponentSystem.h"
#include "scene/SceneSerializer.h"
#include "components/PhysicsComponent.h"
#include "components/VehicleComponent.h"
#include "components/TransformComponent.h"
#include "scripting/ScriptShared.h"

#include <PxPhysicsAPI.h>
#include <vehicle2/PxVehicleAPI.h>

namespace physx {
	class PxScene;
	class PxBroadPhase;
	class PxAABBManager;
	class PxCpuDispatcher;
	class PxMaterial;
	class PxTriangleMesh;
	class PxControllerManager;

}

namespace YAML {
	class Emitter;
	class Node;
}

namespace ORNG {
	struct PhysicsCollisionEvent : public Events::Event {
		PhysicsCollisionEvent(SceneEntity* _p0, SceneEntity* _p1) : p0(_p0), p1(_p1) {};
		SceneEntity* p0 = nullptr;
		SceneEntity* p1 = nullptr;
	};

	class PhysicsSystem : public ComponentSystem {
		friend class EditorLayer;
	public:
		explicit PhysicsSystem(Scene* p_scene);
		virtual ~PhysicsSystem() = default;

		void OnUpdate() override;
		void OnUnload() override;
		void OnLoad() override;
		physx::PxTriangleMesh* GetOrCreateTriangleMesh(const MeshAsset* p_mesh_data);

		RaycastResults Raycast(glm::vec3 origin, glm::vec3 unit_dir, float max_distance);
		OverlapQueryResults OverlapQuery(PxGeometry& geom, glm::vec3 pos, unsigned max_hits);

		bool InitVehicle(VehicleComponent* p_comp);

		void SetIsUpdating(bool is_updating) {
			m_is_updating = is_updating;
		}

		// Returns pointer to list of active actors of length nb_actors.
		// Actors entity can be reached as: static_cast<SceneEntity*>(active_actors[i]->userData)
		PxActor** GetActiveActors(physx::PxU32& nb_actors) {
			return mp_phys_scene->getActiveActors(nb_actors);
		}

		inline static constexpr uint64_t GetSystemUUID() { return 29348475677; }

		enum class ActorType : uint8_t {
			RIGID_BODY,
			CHARACTER_CONTROLLER,
			VEHICLE
		};

		float step_size = 1.f / 90.f;

	private:
		void Tick();

		static void UpdateTransformCompFromGlobalPose(const PxTransform& pose, TransformComponent& transform, PhysicsSystem::ActorType type);

		void InitComponent(PhysicsComponent* p_comp);
		void InitComponent(CharacterControllerComponent* p_comp);
		void InitComponent(VehicleComponent* p_comp);
		void InitComponent(JointComponent* p_comp);

		void HandleComponentUpdate(const Events::ECS_Event<JointComponent>& t_event);
		void UpdateComponentState(PhysicsComponent* p_comp);
		void OnTransformEvent(const Events::ECS_Event<TransformComponent>& t_event);

		void RemoveComponent(PhysicsComponent* p_comp);
		void RemoveComponent(CharacterControllerComponent* p_comp);
		void RemoveComponent(JointComponent* p_comp);
		void RemoveComponent(VehicleComponent* p_comp);

		void ConnectJoint(const JointComponent::ConnectionData& connection);
		void BreakJoint(JointComponent::Joint* p_joint);

		void InitVehicleSimulationContext();

		void InitListeners();
		void DeinitListeners();

		void SerializeEntity(SceneEntity& entity, YAML::Emitter* p_emitter);
		void DeserializeEntity(SceneEntity& entity, YAML::Node* p_node);
		void ResolveJointConnections();
		void RemapJointConnections(SceneEntity& entity, const std::unordered_map<uint64_t, uint64_t>* p_uuid_lookup);

		std::array<entt::connection, 8> m_connections;

		physx::vehicle2::PxVehiclePhysXSimulationContext m_vehicle_context;
		PxConvexMesh* mp_sweep_mesh = nullptr;

		bool m_is_updating = true;

		Events::ECS_EventListener<PhysicsComponent> m_phys_listener;
		Events::ECS_EventListener<JointComponent> m_joint_listener;
		Events::ECS_EventListener<CharacterControllerComponent> m_character_controller_listener;
		Events::ECS_EventListener<TransformComponent> m_transform_listener;
		Events::ECS_EventListener<VehicleComponent> m_vehicle_listener;

		Events::EventListener<EntitySerializationEvent> m_serialization_listener;

		physx::PxBroadPhase* mp_broadphase = nullptr;
		physx::PxAABBManager* mp_aabb_manager = nullptr;
		physx::PxScene* mp_phys_scene = nullptr;
		physx::PxControllerManager* mp_controller_manager = nullptr;

		std::unordered_map<const MeshAsset*, physx::PxTriangleMesh*> m_triangle_meshes;

		// Joints that have been broken during the simulation and logged with onConstraintBreak are stored here to disconnect them from entities after simulate() has finished
		std::vector<JointComponent::Joint*> m_joints_to_break;

		// Need to store joint currently being processed by BreakJoint() as it will trigger onConstraintBreak which will call BreakJoint() again if this isn't checked
		JointComponent::Joint* mp_currently_breaking_joint = nullptr;

		// Queue of entities that need OnCollision script events (if they have one) to fire, has to be done outside of simulation due to restrictions with rigidbody modification during simulation, processed each frame in OnUpdate
		std::vector<std::pair<entt::entity, entt::entity>> m_entity_collision_queue;

		enum TriggerEvent {
			ENTERED,
			EXITED
		};

		std::vector<std::tuple<TriggerEvent, entt::entity, entt::entity>> m_trigger_event_queue;

		// Transform that is currently being updated by the physics system, used to prevent needless physics component updates
		TransformComponent* mp_currently_updating_transform = nullptr;

		float m_accumulator = 0.f;

		class PhysCollisionCallback : public physx::PxSimulationEventCallback {
		public:
			PhysCollisionCallback(PhysicsSystem* p_system) : mp_system(p_system) {};

			virtual void onConstraintBreak([[maybe_unused]] PxConstraintInfo* constraints, [[maybe_unused]] PxU32 count) override;

			virtual void onWake([[maybe_unused]] PxActor** actors, [[maybe_unused]] PxU32 count) override {};

			virtual void onSleep [[maybe_unused]] (PxActor** actors, [[maybe_unused]] PxU32 count) override {};

			virtual void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;

			virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) override;

			virtual void onAdvance([[maybe_unused]] const PxRigidBody* const* bodyBuffer, [[maybe_unused]] const PxTransform* poseBuffer, [[maybe_unused]] const PxU32 count) override {};


		private:
			// Used for its PxActor entity lookup map
			PhysicsSystem* mp_system = nullptr;
		};

		PhysCollisionCallback m_collision_callback{ this };
	};
}