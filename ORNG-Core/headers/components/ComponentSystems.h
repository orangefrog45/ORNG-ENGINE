#pragma once
#include "components/ComponentAPI.h"
#include "events/EventManager.h"
#include "rendering/Textures.h"

#include "physx/PxPhysicsAPI.h"
#include "physx/vehicle2/PxVehicleAPI.h"

#include "scripting/ScriptShared.h"
#include "components/ParticleBufferComponent.h"
#include "scene/Scene.h"

namespace physx {
	class PxScene;
	class PxBroadPhase;
	class PxAABBManager;
	class PxCpuDispatcher;
	class PxMaterial;
	class PxTriangleMesh;
	class PxControllerManager;
}

namespace FMOD {
	class ChannelGroup;
}

namespace ORNG {
	class Scene;
	class MeshInstanceGroup;
	class ShaderVariants;

	// Classes that inherit from this class MUST implement the function "static constexpr uint64_t GetSystemUUID()" which returns a UUID unique to that class
	class ComponentSystem {
	public:
		explicit ComponentSystem(Scene* p_scene) : mp_scene(p_scene) {};
		// Dispatches event attached to single component, used for connections with entt::registry::on_construct etc
		template<std::derived_from<Component> T>
		static void DispatchComponentEvent(entt::registry& registry, entt::entity entity, Events::ECS_EventType type) {
			Events::ECS_Event<T> e_event{ type, &registry.get<T>(entity) };
			Events::EventManager::DispatchEvent(e_event);
		}

		virtual void OnUpdate() {};

		virtual void OnLoad() {};

		virtual void OnUnload() {};

		inline uint64_t GetSceneUUID() const { return mp_scene->uuid(); }
	protected:
		Scene* mp_scene = nullptr;
	};



	class AudioSystem : public ComponentSystem {
	public:
		AudioSystem(Scene* p_scene) : ComponentSystem(p_scene) {};
		void OnLoad();
		void OnUnload();
		void OnUpdate();
		inline static constexpr uint64_t GetSystemUUID() { return 8881736346456; }
	private:
		void OnAudioDeleteEvent(const Events::ECS_Event<AudioComponent>& e_event);
		void OnAudioUpdateEvent(const Events::ECS_Event<AudioComponent>& e_event);
		void OnAudioAddEvent(const Events::ECS_Event<AudioComponent>& e_event);
		void OnTransformEvent(const Events::ECS_Event<TransformComponent>& e_event);


		Events::ECS_EventListener<AudioComponent> m_audio_listener;
		Events::ECS_EventListener<TransformComponent> m_transform_listener;

		FMOD::ChannelGroup* mp_channel_group = nullptr;

		// Points to memory in scene's "CameraSystem" to find active camera
		entt::entity* mp_active_cam_id;
	};



	class TransformHierarchySystem : public ComponentSystem {
	public:
		TransformHierarchySystem(Scene* p_scene) : ComponentSystem(p_scene) {};
		void OnLoad() override;

		void OnUnload() override {
			Events::EventManager::DeregisterListener((entt::entity)m_transform_event_listener.GetRegisterID());
		}

		inline static constexpr uint64_t GetSystemUUID() { return 934898474626; }

	private:
		void UpdateChildTransforms(const Events::ECS_Event<TransformComponent>&);
		Events::ECS_EventListener<TransformComponent> m_transform_event_listener;
	};


	class CameraSystem : public ComponentSystem {
		friend class Scene;
	public:
		CameraSystem(Scene* p_scene) : ComponentSystem(p_scene) {
			m_event_listener.OnEvent = [this](const Events::ECS_Event<CameraComponent>& t_event) {
				if (t_event.event_type == Events::ECS_EventType::COMP_UPDATED && t_event.p_component->is_active) {
					SetActiveCamera(t_event.p_component->GetEnttHandle());
				}
				};

			m_event_listener.scene_id = GetSceneUUID();
			Events::EventManager::RegisterListener(m_event_listener);
		};
		virtual ~CameraSystem() = default;

		void OnUpdate() override {
			auto* p_active_cam = GetActiveCamera();
			if (p_active_cam)
				p_active_cam->Update();
		}

		void SetActiveCamera(entt::entity entity_handle) {
			m_active_cam_entity_handle = entity_handle;
			auto view = mp_scene->GetRegistry().view<CameraComponent>();

			// Make all other cameras inactive
			for (auto [entity, camera] : view.each()) {
				if (entity != entity_handle)
					camera.is_active = false;
			}
		}

		// Returns ptr to active camera or nullptr if no camera is active.
		CameraComponent* GetActiveCamera() {
			auto& reg = mp_scene->GetRegistry();

			if (reg.all_of<CameraComponent>((entt::entity)m_active_cam_entity_handle))
				return &reg.get<CameraComponent>((entt::entity)m_active_cam_entity_handle);
			else
				return nullptr;
		}

		inline static constexpr uint64_t GetSystemUUID() { return 19394857567; }


	private:
		entt::entity m_active_cam_entity_handle;
		Events::ECS_EventListener<CameraComponent> m_event_listener;
	};



	struct PhysicsCollisionEvent : public Events::Event {
		PhysicsCollisionEvent(SceneEntity* _p0, SceneEntity* _p1) : p0(_p0), p1(_p1) {};
		SceneEntity* p0 = nullptr;
		SceneEntity* p1 = nullptr;
	};

	class PhysicsSystem : public ComponentSystem {
		friend class EditorLayer;
	public:
		PhysicsSystem(Scene* p_scene);
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
	private:
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

		std::array<entt::connection, 8> m_connections;

		physx::vehicle2::PxVehiclePhysXSimulationContext m_vehicle_context;
		PxConvexMesh* mp_sweep_mesh = nullptr;

		bool m_is_updating = true;

		Events::ECS_EventListener<PhysicsComponent> m_phys_listener;
		Events::ECS_EventListener<JointComponent> m_joint_listener;
		Events::ECS_EventListener<CharacterControllerComponent> m_character_controller_listener;
		Events::ECS_EventListener<TransformComponent> m_transform_listener;
		Events::ECS_EventListener<VehicleComponent> m_vehicle_listener;


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

		static constexpr float m_step_size = (1.f / 60.f);
		float m_accumulator = 0.f;

		class PhysCollisionCallback : public physx::PxSimulationEventCallback {
		public:
			PhysCollisionCallback(PhysicsSystem* p_system) : mp_system(p_system) {};

			virtual void onConstraintBreak([[maybe_unused]] PxConstraintInfo* constraints, [[maybe_unused]] PxU32 count) override;

			virtual void onWake([[maybe_unused]] PxActor** actors, [[maybe_unused]] PxU32 count) override {};

			virtual void onSleep[[maybe_unused]] (PxActor** actors, [[maybe_unused]] PxU32 count) override {};

			virtual void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;

			virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) override;

			virtual void onAdvance([[maybe_unused]] const PxRigidBody* const* bodyBuffer, [[maybe_unused]] const PxTransform* poseBuffer, [[maybe_unused]] const PxU32 count) override {};


		private:
			// Used for its PxActor entity lookup map
			PhysicsSystem* mp_system = nullptr;
		};

		PhysCollisionCallback m_collision_callback{ this };
	};


	class MeshInstancingSystem : public ComponentSystem {
	public:
		MeshInstancingSystem(Scene* p_scene);
		virtual ~MeshInstancingSystem() = default;
		void SortMeshIntoInstanceGroup(MeshComponent* comp);
		void OnLoad() override;
		void OnUnload() override;
		void OnUpdate() override;
		void OnMeshEvent(const Events::ECS_Event<MeshComponent>& t_event);
		void OnTransformEvent(const Events::ECS_Event<TransformComponent>& t_event);

		const auto& GetInstanceGroups() const { return m_instance_groups; }
		const auto& GetBillboardInstanceGroups() const { return m_billboard_instance_groups; }

		inline static constexpr uint64_t GetSystemUUID() { return 7828929347847; }

	private:
		void OnMeshAssetDeletion(MeshAsset* p_asset);
		void OnMaterialDeletion(Material* p_material);

		void SortBillboardIntoInstanceGroup(BillboardComponent* p_comp);
		void OnBillboardAdd(BillboardComponent* p_comp);
		void OnBillboardRemove(BillboardComponent* p_comp);

		// Listener for asset deletion
		Events::EventListener<Events::AssetEvent> m_asset_listener;

		Events::ECS_EventListener<TransformComponent> m_transform_listener;
		Events::ECS_EventListener<MeshComponent> m_mesh_listener;
		std::vector<MeshInstanceGroup*> m_instance_groups;

		Events::ECS_EventListener<BillboardComponent> m_billboard_listener;
		std::vector<MeshInstanceGroup*> m_billboard_instance_groups;

		entt::connection m_mesh_add_connection;
		entt::connection m_mesh_remove_connection;
		entt::connection m_billboard_add_connection;
		entt::connection m_billboard_remove_connection;

		unsigned m_default_group_end_index = 0;
	};

	class ParticleBufferComponent;

	class Shader;
	class ShaderVariants;

	class ParticleSystem : public ComponentSystem {
		friend class EditorLayer;
		friend class SceneRenderer;
	public:
		ParticleSystem(Scene* p_scene);
		void OnLoad() override;
		void OnUnload() override;
		void OnUpdate() override;

		inline static constexpr uint64_t GetSystemUUID() { return 1927773874672; }

	private:
		void InitEmitter(ParticleEmitterComponent* p_comp);
		void InitParticles(ParticleEmitterComponent* p_comp);
		void OnEmitterUpdate(const Events::ECS_Event<ParticleEmitterComponent>& e_event);
		void OnEmitterUpdate(ParticleEmitterComponent* p_comp);
		void OnEmitterDestroy(ParticleEmitterComponent* p_comp, unsigned dif = 0);

		void InitBuffer(ParticleBufferComponent* p_comp);
		void OnBufferDestroy(ParticleBufferComponent* p_comp);
		void OnBufferUpdate(ParticleBufferComponent* p_comp);

		void OnEmitterVisualTypeChange(ParticleEmitterComponent* p_comp);

		void UpdateEmitterBufferAtIndex(unsigned index);

		std::array<entt::connection, 4> m_connections;

		Events::ECS_EventListener<ParticleEmitterComponent> m_particle_listener;
		Events::ECS_EventListener<ParticleBufferComponent> m_particle_buffer_listener;

		Events::ECS_EventListener<TransformComponent> m_transform_listener;

		// Stored in order based on their m_particle_start_index
		std::vector<entt::entity> m_emitter_entities;

		// Total particles belonging to emitters, does not include those belonging to ParticleBufferComponents which are stored separately
		unsigned total_emitter_particles = 0;

		SSBO<float> m_particle_ssbo{ false, 0 };
		SSBO<float> m_emitter_ssbo{ false, GL_DYNAMIC_STORAGE_BIT };

		inline static Shader* mp_particle_cs;
		inline static ShaderVariants* mp_particle_initializer_cs;
	};


	struct BillboardInstanceGroup {
		unsigned num_instances = 0;
		Material* p_material = nullptr;

		// Transforms stored as two vec3's (pos, scale)
		SSBO<float> transform_ssbo;
	};
}