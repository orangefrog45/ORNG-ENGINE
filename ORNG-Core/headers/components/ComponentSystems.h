#pragma once
#include "components/ComponentAPI.h"
#include "events/EventManager.h"
#include "rendering/Textures.h"
#include "physx/PxPhysicsAPI.h"
#include "physx/vehicle2/PxVehicleAPI.h"
#include "scripting/ScriptShared.h"

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

using namespace physx;

namespace ORNG {
	class Scene;
	class MeshInstanceGroup;

	class ComponentSystem {
	public:
		explicit ComponentSystem(uint64_t scene_uuid) : m_scene_uuid(scene_uuid) {};
		// Dispatches event attached to single component, used for connections with entt::registry::on_construct etc
		template<std::derived_from<Component> T>
		static void DispatchComponentEvent(entt::registry& registry, entt::entity entity, Events::ECS_EventType type) {
			Events::ECS_Event<T> e_event;
			e_event.affected_components[0] = &registry.get<T>(entity);
			e_event.event_type = type;

			Events::EventManager::DispatchEvent(e_event);
		}

		inline uint64_t GetSceneUUID() const { return m_scene_uuid; }
	private:
		uint64_t m_scene_uuid = 0;
	};



	class AudioSystem : public ComponentSystem {
	public:
		AudioSystem(entt::registry* p_registry, uint64_t scene_uuid, entt::entity* p_active_cam_id) : ComponentSystem(scene_uuid), mp_registry(p_registry), mp_active_cam_id(p_active_cam_id) {};
		void OnLoad();
		void OnUnload();
		void OnUpdate();
	private:

		void OnAudioDeleteEvent(const Events::ECS_Event<AudioComponent>& e_event);
		void OnAudioUpdateEvent(const Events::ECS_Event<AudioComponent>& e_event);
		void OnAudioAddEvent(const Events::ECS_Event<AudioComponent>& e_event);
		void OnTransformEvent(const Events::ECS_Event<TransformComponent>& e_event);

		Events::ECS_EventListener<AudioComponent> m_audio_listener;
		Events::ECS_EventListener<TransformComponent> m_transform_listener;

		FMOD::ChannelGroup* mp_channel_group = nullptr;
		entt::registry* mp_registry = nullptr;

		// Points to memory in scene's "CameraSystem" to find active camera
		entt::entity* mp_active_cam_id;
	};



	class TransformHierarchySystem : public ComponentSystem {
	public:
		TransformHierarchySystem(entt::registry* p_registry, uint64_t scene_uuid) : ComponentSystem(scene_uuid), mp_registry(p_registry) {};
		void OnLoad();

		void OnUnload() {
			Events::EventManager::DeregisterListener((entt::entity)m_transform_event_listener.GetRegisterID());
		}
	private:
		void UpdateChildTransforms(const Events::ECS_Event<TransformComponent>&);
		Events::ECS_EventListener<TransformComponent> m_transform_event_listener;
		entt::registry* mp_registry = nullptr;
	};


	class CameraSystem : public ComponentSystem {
		friend class Scene;
	public:
		CameraSystem(entt::registry* p_registry, uint64_t scene_uuid) : ComponentSystem(scene_uuid), mp_registry(p_registry) {
			m_event_listener.OnEvent = [this](const Events::ECS_Event<CameraComponent>& t_event) {
				if (t_event.event_type == Events::ECS_EventType::COMP_UPDATED && t_event.affected_components[0]->is_active) {
					SetActiveCamera(t_event.affected_components[0]->GetEnttHandle());
				}
				};

			m_event_listener.scene_id = GetSceneUUID();
			Events::EventManager::RegisterListener(m_event_listener);
		};
		virtual ~CameraSystem() = default;

		void OnUnload() {};
		void OnLoad() {
		}

		void OnUpdate() {
			auto* p_active_cam = GetActiveCamera();
			if (p_active_cam)
				p_active_cam->Update();
		}

		void SetActiveCamera(entt::entity entity_handle) {
			m_active_cam_entity_handle = entity_handle;
			auto view = mp_registry->view<CameraComponent>();

			// Make all other cameras inactive
			for (auto [entity, camera] : view.each()) {
				if (entity != entity_handle)
					camera.is_active = false;
			}
		}

		// Returns ptr to active camera or nullptr if no camera is active.
		CameraComponent* GetActiveCamera() {
			if (mp_registry->all_of<CameraComponent>((entt::entity)m_active_cam_entity_handle))
				return &mp_registry->get<CameraComponent>((entt::entity)m_active_cam_entity_handle);
			else
				return nullptr;
		}

	private:
		entt::entity m_active_cam_entity_handle;
		Events::ECS_EventListener<CameraComponent> m_event_listener;
		entt::registry* mp_registry = nullptr;
	};






	class PhysicsSystem : public ComponentSystem {
		friend class EditorLayer;
	public:
		PhysicsSystem(entt::registry* p_registry, uint64_t scene_uuid, Scene* p_scene);
		virtual ~PhysicsSystem() = default;

		void OnUpdate(float ts);
		void OnUnload();
		void OnLoad();
		const auto& GetMaterials() { return m_physics_materials; }
		physx::PxTriangleMesh* GetOrCreateTriangleMesh(const MeshAsset* p_mesh_data);

		RaycastResults Raycast(glm::vec3 origin, glm::vec3 unit_dir, float max_distance);


		// Returns ptr to entity containing the physics component that has p_actor or nullptr if no matches found
		SceneEntity* TryGetEntityFromPxActor(const physx::PxActor* p_actor) {
			if (m_entity_lookup.contains(p_actor))
				return m_entity_lookup[p_actor];
			else
				return nullptr;
		}

		bool InitVehicle(VehicleComponent* p_comp);


	private:
		void InitComponent(PhysicsComponent* p_comp);
		void InitComponent(CharacterControllerComponent* p_comp);
		void InitComponent(VehicleComponent* p_comp);
		void HandleComponentUpdate(const Events::ECS_Event<FixedJointComponent>& t_event);
		void UpdateComponentState(PhysicsComponent* p_comp);
		void RemoveComponent(PhysicsComponent* p_comp);
		void RemoveComponent(CharacterControllerComponent* p_comp);
		void RemoveComponent(FixedJointComponent* p_comp);
		void RemoveComponent(VehicleComponent* p_comp);


		void InitVehicleSimulationContext();

		physx::vehicle2::PxVehiclePhysXSimulationContext m_vehicle_context;
		PxConvexMesh* mp_sweep_mesh = nullptr;

		void InitListeners();
		void DeinitListeners();


		void OnTransformEvent(const Events::ECS_Event<TransformComponent>& t_event);


		Scene* mp_scene = nullptr;
		entt::registry* mp_registry = nullptr;
		Events::ECS_EventListener<PhysicsComponent> m_phys_listener;
		Events::ECS_EventListener<FixedJointComponent> m_joint_listener;
		Events::ECS_EventListener<CharacterControllerComponent> m_character_controller_listener;
		Events::ECS_EventListener<TransformComponent> m_transform_listener;
		Events::ECS_EventListener<VehicleComponent> m_vehicle_listener;

		physx::PxBroadPhase* mp_broadphase = nullptr;
		physx::PxAABBManager* mp_aabb_manager = nullptr;
		physx::PxScene* mp_phys_scene = nullptr;
		physx::PxControllerManager* mp_controller_manager = nullptr;

		std::vector<physx::PxMaterial*> m_physics_materials;
		std::unordered_map<const MeshAsset*, physx::PxTriangleMesh*> m_triangle_meshes;

		// Quick way to find an entity from its corresponding phys comps PxRigidActor
		std::unordered_map<const physx::PxActor*, SceneEntity*> m_entity_lookup;
		// Queue of entities that need OnCollision script events (if they have one) to fire, has to be done outside of simulation due to restrictions with rigidbody modification during simulation, processed each frame in OnUpdate
		std::vector<std::pair<SceneEntity*, SceneEntity*>> m_entity_collision_queue;

		enum TriggerEvent {
			ENTERED,
			EXITED
		};

		std::vector<std::tuple<TriggerEvent, SceneEntity*, SceneEntity*>> m_trigger_event_queue;





		// Transform that is currently being updated by the physics system, used to prevent needless physics component updates
		TransformComponent* mp_currently_updating_transform = nullptr;

		float m_step_size = (1.f / 60.f);
		float m_accumulator = 0.f;

		class PhysCollisionCallback : public physx::PxSimulationEventCallback {
		public:
			PhysCollisionCallback(PhysicsSystem* p_system) : mp_system(p_system) {};
			virtual void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override {};

			virtual void onWake(PxActor** actors, PxU32 count) override {};

			virtual void onSleep(PxActor** actors, PxU32 count) override {};

			virtual void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;

			virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) override;

			virtual void onAdvance(const PxRigidBody* const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) override {};


		private:
			// Used for its PxActor entity lookup map
			PhysicsSystem* mp_system = nullptr;
		};

		PhysCollisionCallback m_collision_callback{ this };
	};


	class SpotlightSystem {
		friend class SceneRenderer;
	public:
		SpotlightSystem() = default;
		virtual ~SpotlightSystem() = default;

		void OnLoad();
		void OnUpdate(entt::registry* p_registry);
		void OnUnload();
	private:
		void WriteLightToVector(std::vector<float>& output_vec, SpotLightComponent& light, int& index);
		Texture2DArray m_spotlight_depth_tex{
		"Spotlight depth"
		}; // Used for shadow maps
		unsigned int m_shadow_spotlight_ssbo_handle;
		unsigned int m_shadowless_spotlight_ssbo_handle;
	};



	class PointlightSystem {
		friend class SceneRenderer;
	public:
		PointlightSystem() = default;
		~PointlightSystem() = default;
		void OnLoad();
		void OnUpdate(entt::registry* p_registry);
		void OnUnload();
		void WriteLightToVector(std::vector<float>& output_vec, PointLightComponent& light, int& index);

		// Checks if the depth map array needs to grow/shrink
		void OnDepthMapUpdate();
	private:
		TextureCubemapArray m_pointlight_depth_tex{ "Pointlight depth" }; // Used for shadow maps
		unsigned int m_shadowless_pointlight_ssbo_handle;
		unsigned int m_shadow_pointlight_ssbo_handle;
	};


	class MeshInstancingSystem : public ComponentSystem {
	public:

		MeshInstancingSystem(entt::registry* p_registry, uint64_t scene_uuid);
		virtual ~MeshInstancingSystem() = default;
		void SortMeshIntoInstanceGroup(MeshComponent* comp);
		void OnLoad();
		void OnUnload();
		void OnUpdate();
		void OnMeshEvent(const Events::ECS_Event<MeshComponent>& t_event);
		void OnTransformEvent(const Events::ECS_Event<TransformComponent>& t_event);

		const auto& GetInstanceGroups() const { return m_instance_groups; }
	private:
		void OnMeshAssetDeletion(MeshAsset* p_asset);
		void OnMaterialDeletion(Material* p_material);
		// Listener for asset deletion
		Events::EventListener<Events::AssetEvent> m_asset_listener;

		Events::ECS_EventListener<TransformComponent> m_transform_listener;
		Events::ECS_EventListener<MeshComponent> m_mesh_listener;
		std::vector<MeshInstanceGroup*> m_instance_groups;

		unsigned m_default_group_end_index = 0;

		entt::registry* mp_registry = nullptr;
	};
}