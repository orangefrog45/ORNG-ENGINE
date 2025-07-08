#pragma once

#include "components/CameraComponent.h"
#include "components/systems/ComponentSystem.h"
#include "scene/Scene.h"

namespace ORNG {
	class CameraSystem : public ComponentSystem {
		friend class Scene;
	public:
		explicit CameraSystem(Scene* p_scene) : ComponentSystem(p_scene) {
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

		void OnUnload() override {
			Events::EventManager::DeregisterListener(m_event_listener.GetRegisterID());
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
}