#pragma once
#include "events/Events.h"
#include "util/Log.h"
#include "util/util.h"
#include "../extern/entt/EnttSingleInclude.h"

namespace ORNG::Events {

	class EventManager {
	public:

		~EventManager() {
			m_registry.clear();
		}

		template <std::derived_from<Event> T>
		inline static void DispatchEvent(const T& t_event) {
			Get().IDispatchEvent(t_event);
		}


		template <std::derived_from<Event> T>
		inline static void RegisterListener(EventListener<T>& listener) {
			Get().IRegisterListener(listener);
		};

		inline static void DeregisterListener(uint32_t listener_entt_handle) {
			Get().IDeregisterListener(listener_entt_handle);
		};

	private:
		static EventManager& Get() {
			static EventManager s_instance;
			return s_instance;
		}

		template <std::derived_from<Event> T>
		void IRegisterListener(EventListener<T>& listener) {

			if (!listener.OnEvent) {
				ORNG_CORE_ERROR("Failed registering listener, OnEvent callback is nullptr");
				return;
			}

			// Copy of listener stored instead of pointer for faster iteration.
			auto entity = m_registry.create();
			EventListener<T>& reg_listener = m_registry.emplace<EventListener<T>>(entity);
			listener.m_entt_handle = (uint32_t)entity;
			reg_listener.m_entt_handle = listener.m_entt_handle;
			reg_listener.OnEvent = listener.OnEvent;

			// For safety, upon listener being destroyed the copy is too.
			listener.OnDestroy = [this, &listener, entity] {
				EventManager::DeregisterListener((uint32_t)entity);
			};

		};

		void IDeregisterListener(uint32_t entt_handle) {
			if (m_registry.valid(entt::entity(entt_handle)))
				m_registry.destroy(entt::entity(entt_handle));
		};



		template <std::derived_from<Event> T>
		void IDispatchEvent(const T& t_event) {
			auto view = m_registry.view<EventListener<T>>();
			// Iterate over listeners and call callbacks
			for (auto [entity, listener] : view.each()) {
				listener.OnEvent(t_event);
			}

		}

		// Stores event listeners
		entt::registry m_registry;

	};

}