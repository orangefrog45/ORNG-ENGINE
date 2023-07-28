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
}

namespace ORNG {
	class Scene;

	class ComponentSystem {
	public:
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
	};


	class CameraSystem : public ComponentSystem {
	public:

		void OnUnload() {};
		void OnLoad() {}

		void OnUpdate() {
			auto* p_active_cam = GetActiveCamera();
			if (p_active_cam)
				p_active_cam->Update();

		}
		explicit CameraSystem(entt::registry* p_registry) : mp_registry(p_registry) {
			m_event_listener.OnEvent = [this](const Events::ECS_Event<CameraComponent>& t_event) {
				if (t_event.event_type == Events::ECS_EventType::COMP_UPDATED && t_event.affected_components[0]->is_active) {
					SetActiveCamera(t_event.affected_components[0]->GetEnttHandle());
				}
			};
		};

		void SetActiveCamera(uint32_t entity_handle) {
			m_active_cam_entity_handle = entity_handle;
			auto view = mp_registry->view<CameraComponent>();

			// Make all other cameras inactive
			for (auto [entity, camera] : view.each()) {
				if ((uint32_t)entity != entity_handle)
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
		uint32_t m_active_cam_entity_handle;
		Events::EventListener<Events::ECS_Event<CameraComponent>> m_event_listener;
		entt::registry* mp_registry = nullptr;
	};


	class PhysicsSystem : public ComponentSystem {
	public:
		explicit PhysicsSystem(entt::registry* p_registry);

		void OnUpdate(float ts);
		void OnUnload() final;
		void OnLoad();
		physx::PxTriangleMesh* GetOrCreateTriangleMesh(const MeshAsset* p_mesh_data);


		bool GetIsPaused() const { return m_physics_paused; };
		void SetIsPaused(bool v) { m_physics_paused = v; };

	private:
		void InitComponent(PhysicsComponent* p_comp);
		void UpdateComponentState(PhysicsComponent* p_comp);
		void RemoveComponent(PhysicsComponent* p_comp);

		entt::registry* mp_registry = nullptr;
		Events::EventListener<Events::ECS_Event<PhysicsComponent>> m_phys_listener;
		Events::EventListener<Events::ECS_Event<TransformComponent>> m_transform_listener;

		physx::PxBroadPhase* mp_broadphase = nullptr;
		physx::PxAABBManager* mp_aabb_manager = nullptr;
		physx::PxScene* mp_scene = nullptr;

		std::vector<physx::PxMaterial*> m_physics_materials;
		std::unordered_map<const MeshAsset*, physx::PxTriangleMesh*> m_triangle_meshes;

		bool m_physics_paused = true;
		float m_step_size = 1.f / 60.f;
		float m_accumulator = 0.f;


	};


	class SpotlightSystem : public ComponentSystem {
		friend class SceneRenderer;
	public:
		explicit SpotlightSystem(entt::registry* p_registry) : mp_registry(p_registry) {};

		void OnLoad();
		void OnUpdate() final;
		void OnUnload() final;
	private:
		entt::registry* mp_registry = nullptr;
		GLuint m_spotlight_ssbo_handle;
	};



	class PointlightSystem : public ComponentSystem {
		friend class SceneRenderer;
	public:
		explicit PointlightSystem(entt::registry* p_registry) : mp_registry(p_registry) {};
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

		explicit MeshInstancingSystem(entt::registry* p_registry);
		void SortMeshIntoInstanceGroup(MeshComponent* comp);
		void OnLoad();
		void OnUnload() final;
		void OnUpdate() final;
		void OnMeshEvent(const Events::ECS_Event<MeshComponent>& t_event);
		void OnTransformEvent(const Events::ECS_Event<TransformComponent>& t_event);
		void OnMeshAssetDeletion(MeshAsset* p_asset);
		void OnMaterialDeletion(Material* p_material, Material* p_replacement_material);

		const auto& GetInstanceGroups() const { return m_instance_groups; }
	private:
		Events::EventListener<Events::ECS_Event<TransformComponent>> m_transform_listener;
		Events::EventListener<Events::ECS_Event<MeshComponent>> m_mesh_listener;
		std::vector<MeshInstanceGroup*> m_instance_groups;
		entt::registry* mp_registry = nullptr;
	};
}