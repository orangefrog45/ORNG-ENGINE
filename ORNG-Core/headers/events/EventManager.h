#pragma once
#include "util/Log.h"
#include "util/util.h"
#include "../extern/entt/EnttSingleInclude.h"
#include "events/Events.h"



namespace ORNG::Events {


	class EventManager {
	public:

		~EventManager() {
			m_registry.clear();
		}



		template <std::derived_from<Event> T>
		inline static void RegisterListener(EventListener<T>& listener) {
			if (!listener.OnEvent) {
				ORNG_CORE_ERROR("Failed registering listener, OnEvent callback is nullptr");
				return;
			}

			// Copy of listener stored instead of pointer for faster iteration.
			auto entity = Get().m_registry.create();
			listener.m_entt_handle = (uint32_t)entity;
			Get().m_registry.emplace<EventListener<T>>(entity, listener);


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
			auto entity = Get().m_registry.create();
			listener.m_entt_handle = (uint32_t)entity;
			Get().m_registry.emplace<ECS_EventListener<T>>(entity, listener);

			// For safety, upon listener being destroyed the copy is too.
			listener.OnDestroy = [&listener, entity] {
				EventManager::DeregisterListener(entity);
			};
		};


		template <std::derived_from<Event> T>
		static void DispatchEvent(const T& t_event) {
			// Iterate over listeners and call callbacks
			for (auto [entity, listener] : Get().m_registry.view<EventListener<T>>().each()) {
				listener.OnEvent(t_event);
			}

		}

		template<std::derived_from<Component> T>
		static void DispatchEvent(const ECS_Event<T>& t_event) {
			// Iterate over listeners and call callbacks
			for (auto [entity, listener] : Get().m_registry.view<ECS_EventListener<T>>().each()) {
				// Check if the event listener is listening to the scene the event was dispatched from
				if (listener.scene_id == static_cast<Component*>(t_event.affected_components[0])->GetSceneUUID())
					listener.OnEvent(t_event);
			}
		}


		static void DeregisterListener(entt::entity entt_handle) {
			if (Get().m_registry.valid(entt_handle))
				Get().m_registry.destroy(entt_handle);
		};

	private:

		inline static EventManager& Get() {
			static EventManager s_instance;
			return s_instance;
		}

		// Stores event listeners
		entt::registry m_registry;

	};

}