#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H

#include "util/Log.h"
#include "util/util.h"
#include "../extern/entt/EnttSingleInclude.h"
#include "events/Events.h"


namespace ORNG {
	class MeshComponent;
	class SpotLightComponent;
	struct PointLightComponent;
}

namespace ORNG::Events {

	class EventManager {
	public:
		~EventManager() {
			m_listener_registry.clear();
		}

		static void Init() {
			mp_instance = new EventManager();
		}

		template <std::derived_from<Event> T>
		inline static void RegisterListenerType() {
			auto entity = Get().m_listener_registry.create();
			auto& reg = Get().m_listener_registry;
			reg.emplace<EventListener<T>>(entity);
			reg.destroy(entity);
		}

		template <std::derived_from<Event> T>
		inline static void RegisterListener(EventListener<T>& listener) {
			if (!listener.OnEvent) {
				ORNG_CORE_ERROR("Failed registering listener, OnEvent callback is nullptr");
				return;
			}

			// Copy of listener stored instead of pointer for faster iteration.
			auto entity = Get().m_listener_registry.create();
			listener.m_entt_handle = entity;
			Get().m_listener_registry.emplace<EventListener<T>>(entity, listener);


			// For safety, upon listener being destroyed the copy is too.
			listener.OnDestroy = [&listener, entity] {
				EventManager::DeregisterListener(entity);
			};
		};

		template <std::derived_from<Component> T>
		inline static void RegisterListener(ECS_EventListener<T>& listener) {
			if (!listener.OnEvent) {
				ORNG_CORE_ERROR("Failed registering listener, OnEvent callback is nullptr");
				return;
			}
			// Copy of listener stored instead of pointer for faster iteration.
			auto entity = Get().m_listener_registry.create();
			listener.m_entt_handle = entity;
			Get().m_listener_registry.emplace<ECS_EventListener<T>>(entity, listener);

			// For safety, upon listener being destroyed the copy is too.
			listener.OnDestroy = [&listener, entity] {
				EventManager::DeregisterListener(entity);
			};
		};

		static void DispatchEvent(const KeyEvent& t_event) {
			for (auto [entity, listener] : Get().m_listener_registry.view<EventListener<Events::KeyEvent>>().each()) {
				listener.OnEvent(t_event);
			}
		}

		template <std::derived_from<Event> T>
		static void DispatchEvent(const T& t_event) {
			// Iterate over listeners and call callbacks
			for (auto [entity, listener] : Get().m_listener_registry.view<EventListener<T>>().each()) {
				listener.OnEvent(t_event);
			}

		}

		template<std::derived_from<Component> T>
		static void DispatchEvent(const ECS_Event<T>& t_event) {
				for (auto [entity, listener] : Get().m_listener_registry.view<ECS_EventListener<T>>().each()) {
					// Check if the event listener is listening to the scene the event was dispatched from
					if (listener.scene_id == t_event.p_component->GetSceneUUID())
						listener.OnEvent(t_event);
				}
		}


		static void DeregisterListener(entt::entity entt_handle) {
			if (Get().m_listener_registry.valid(entt_handle))
				Get().m_listener_registry.destroy(entt_handle);
		};

		static void SetInstance(EventManager* p_instance) {
			mp_instance = p_instance;
		}

		inline static EventManager& Get() {
			return *mp_instance;
		}

	private:
		inline static EventManager* mp_instance = nullptr;

		// Stores event listeners
		entt::registry m_listener_registry;

	};

}

#endif