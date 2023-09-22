#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H

#include "util/Log.h"
#include "util/util.h"
#include "../extern/entt/EnttSingleInclude.h"
#include "events/Events.h"


namespace ORNG {
	class MeshComponent;
	class SpotLightComponent;
	class PointLightComponent;
}

namespace ORNG::Events {

	class EventManager {
	public:

		~EventManager() {
			m_listener_registry.clear();
			delete mp_instance;
		}

		static void Init() {
			mp_instance = new EventManager();
		}

		static void ProcessQueuedEvents() {
			for (auto& func : Get().m_events_to_process) {
				func();
			}
			Get().m_events_to_process.clear();
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

			// Listener will already have upper-case key (done automatically), regular EventListener<KeyEvent> is case-aware
			char upper_key = std::toupper(t_event.key);
			for (auto [entity, listener] : Get().m_listener_registry.view<SingleKeyEventListener>().each()) {
				if (listener.m_listening_key == upper_key)
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
			auto p_func = [t_event] {
				for (auto [entity, listener] : Get().m_listener_registry.view<ECS_EventListener<T>>().each()) {
					// Check if the event listener is listening to the scene the event was dispatched from
					if (listener.scene_id == static_cast<Component*>(t_event.affected_components[0])->GetSceneUUID())
						listener.OnEvent(t_event);
				}
			};
#ifdef ORNG_SCRIPT_ENV // Push event to queue so it's processed by the main application, not the script itself (needed for opengl operations)
			if constexpr (std::is_same_v<T, ORNG::MeshComponent> || std::is_same_v<T, ORNG::PointLightComponent> || std::is_same_v<T, ORNG::SpotLightComponent>) {
				m_events_to_process.push_back(p_func);
			}
			else {
				// No opengl, safe to handle in script environment
				p_func();
			}
			
#else
			// In-engine event, handle immediately
			p_func();
#endif
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
		std::vector<std::function<void()>> m_events_to_process;

	};

}

#endif