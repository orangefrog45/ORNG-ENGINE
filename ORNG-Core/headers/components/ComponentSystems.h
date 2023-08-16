#pragma once
#include "components/ComponentAPI.h"
#include "scene/MeshInstanceGroup.h"
#include "events/EventManager.h"


namespace physx {
	class PxScene;
	class PxBroadPhase;
	class PxAABBManager;
	class PxCpuDispatcher;
	class PxMaterial;
	class PxTriangleMesh;
	class PxControllerManager;
}

namespace ORNG {
	class Scene;

	class ComponentSystem {
	public:
		explicit ComponentSystem(uint64_t scene_uuid) : m_scene_uuid(scene_uuid) {  };
		// Dispatches event attached to single component, used for connections with entt::registry::on_construct etc
		template<std::derived_from<Component> T>
		static void DispatchComponentEvent(entt::registry& registry, entt::entity entity, Events::ECS_EventType type) {
			Events::ECS_Event<T> e_event;
			e_event.affected_components.push_back(&registry.get<T>(entity));
			e_event.event_type = type;

			Events::EventManager::DispatchEvent(e_event);
		}

		virtual void OnUpdate() {};
		virtual void OnUnload() {};

		inline uint64_t GetSceneUUID() const { return m_scene_uuid; }
	private:
		uint64_t m_scene_uuid = 0;
	};

	class TransformHierarchySystem : public ComponentSystem {
	public:
		TransformHierarchySystem(entt::registry* p_registry, uint64_t scene_uuid) : ComponentSystem(scene_uuid), mp_registry(p_registry) {};
		void OnLoad() {
			// On transform update event, update all child transforms
			m_transform_event_listener.OnEvent = [this](const Events::ECS_Event<TransformComponent>& t_event) {
				[[likely]] if (t_event.event_type == Events::ECS_EventType::COMP_UPDATED) {
					auto& relationship_comp = mp_registry->get<RelationshipComponent>(entt::entity(t_event.affected_components[0]->GetEnttHandle()));
					entt::entity current_entity = relationship_comp.first;

					for (int i = 0; i < relationship_comp.num_children; i++) {
						mp_registry->get<TransformComponent>(current_entity).RebuildMatrix(static_cast<TransformComponent::UpdateType>(t_event.sub_event_type));
						current_entity = mp_registry->get<RelationshipComponent>(current_entity).next;
					}

				}
			};
			m_transform_event_listener.scene_id = GetSceneUUID();
			Events::EventManager::RegisterListener(m_transform_event_listener);
		}

		void OnUnload() {
			Events::EventManager::DeregisterListener((entt::entity)m_transform_event_listener.GetRegisterID());
		}
	private:
		Events::ECS_EventListener<TransformComponent> m_transform_event_listener;
		entt::registry* mp_registry = nullptr;
	};


	class CameraSystem : public ComponentSystem {
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
	public:
		PhysicsSystem(entt::registry* p_registry, uint64_t scene_uuid);
		virtual ~PhysicsSystem() = default;

		void OnUpdate(float ts);
		void OnUnload() final;
		void OnLoad();
		physx::PxTriangleMesh* GetOrCreateTriangleMesh(const MeshAsset* p_mesh_data);


		bool GetIsPaused() const { return m_physics_paused; };
		void SetIsPaused(bool v) { m_physics_paused = v; };

	private:
		void InitComponent(PhysicsComponent* p_comp);
		void InitComponent(CharacterControllerComponent* p_comp);
		void UpdateComponentState(PhysicsComponent* p_comp);
		void RemoveComponent(PhysicsComponent* p_comp);
		void RemoveComponent(CharacterControllerComponent* p_comp);

		entt::registry* mp_registry = nullptr;
		Events::ECS_EventListener<PhysicsComponent> m_phys_listener;
		Events::ECS_EventListener<CharacterControllerComponent> m_character_controller_listener;
		Events::ECS_EventListener<TransformComponent> m_transform_listener;

		physx::PxBroadPhase* mp_broadphase = nullptr;
		physx::PxAABBManager* mp_aabb_manager = nullptr;
		physx::PxScene* mp_scene = nullptr;
		physx::PxControllerManager* mp_controller_manager = nullptr;

		std::vector<physx::PxMaterial*> m_physics_materials;
		std::unordered_map<const MeshAsset*, physx::PxTriangleMesh*> m_triangle_meshes;

		// Transform that is currently being updated by the physics system, used to prevent needless physics component updates
		TransformComponent* mp_currently_updating_transform = nullptr;

		bool m_physics_paused = true;
		float m_step_size = (1.f / 60.f);
		float m_accumulator = 0.f;


	};


	class SpotlightSystem : public ComponentSystem {
		friend class SceneRenderer;
	public:
		SpotlightSystem(entt::registry* p_registry, uint64_t scene_uuid) : ComponentSystem(scene_uuid), mp_registry(p_registry) {};
		virtual ~SpotlightSystem() = default;

		void OnLoad();
		void OnUpdate() final;
		void OnUnload() final;
	private:
		Texture2DArray m_spotlight_depth_tex{ "Spotlight depth" }; // Used for shadow maps
		entt::registry* mp_registry = nullptr;
		GLuint m_spotlight_ssbo_handle;
	};



	class PointlightSystem : public ComponentSystem {
		friend class SceneRenderer;
	public:
		PointlightSystem(entt::registry* p_registry, uint64_t scene_uuid) : ComponentSystem(scene_uuid), mp_registry(p_registry) {};
		virtual ~PointlightSystem() = default;
		void OnLoad();
		void OnUpdate() final;
		void OnUnload() final;

		// Checks if the depth map array needs to grow/shrink
		void OnDepthMapUpdate();
	private:
		TextureCubemapArray m_pointlight_depth_tex{ "Pointlight depth" }; // Used for shadow maps
		GLuint m_pointlight_ssbo_handle;
		entt::registry* mp_registry = nullptr;

	};


	class MeshInstancingSystem : public ComponentSystem {
	public:

		MeshInstancingSystem(entt::registry* p_registry, uint64_t scene_uuid);
		virtual ~MeshInstancingSystem() = default;
		void SortMeshIntoInstanceGroup(MeshComponent* comp);
		void OnLoad();
		void OnUnload() final;
		void OnUpdate() final;
		void OnMeshEvent(const Events::ECS_Event<MeshComponent>& t_event);
		void OnTransformEvent(const Events::ECS_Event<TransformComponent>& t_event);

		const auto& GetInstanceGroups() const { return m_instance_groups; }
	private:
		void OnMeshAssetDeletion(MeshAsset* p_asset);
		void OnMaterialDeletion(Material* p_material);
		// Listener for asset deletion
		Events::EventListener<Events::ProjectEvent> m_asset_listener;

		Events::ECS_EventListener<TransformComponent> m_transform_listener;
		Events::ECS_EventListener<MeshComponent> m_mesh_listener;
		std::vector<MeshInstanceGroup*> m_instance_groups;
		entt::registry* mp_registry = nullptr;
	};
}