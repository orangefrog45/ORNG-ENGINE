#pragma once
#include "events/Events.h"
#include "util/Log.h"
#include "util/util.h"


namespace ORNG::Events {

	class EventManager {
	public:

		template <std::derived_from<Event> T>
		inline static void DispatchEvent(const T& t_event) {
			Get().IDispatchEvent(t_event);
		}


		template <std::derived_from<Event> T>
		inline static void RegisterListener(EventListener<T>& listener) {
			Get().IRegisterListener(listener);
		};

	private:
		static EventManager& Get() {
			static EventManager s_instance;
			return s_instance;
		}

		template <std::derived_from<Event> T>
		void IRegisterListener(EventListener<T>& listener) {

			if (!listener.OnEvent) {
				OAR_CORE_ERROR("Failed registering listener, OnEvent callback is nullptr");
				return;
			}

			bool is_registered = false;

			if constexpr (std::is_same<T, EngineCoreEvent>::value) {
				m_engine_core_listeners.push_back(&listener);
				listener.OnDestroy = [this, &listener] {m_engine_core_listeners.erase(std::find(m_engine_core_listeners.begin(), m_engine_core_listeners.end(), &listener)); };
				is_registered = true;
			}
			else if constexpr (std::is_same<T, WindowEvent>::value) {
				m_window_event_listeners.push_back(&listener);
				listener.OnDestroy = [this, &listener] {m_window_event_listeners.erase(std::find(m_window_event_listeners.begin(), m_window_event_listeners.end(), &listener)); };
				is_registered = true;
			}

			if (!is_registered) {
				OAR_CORE_ERROR("Failed registering listener, unsupported event type.");
			}

		};



		template <std::derived_from<Event> T>
		void IDispatchEvent(const T& t_event) {

			std::vector<const EventListener<T>*>* p_listeners = nullptr;

			// Locate appropiate array for event type
			if constexpr (std::is_same<T, EngineCoreEvent>::value) {
				p_listeners = &m_engine_core_listeners;
			}
			else if constexpr (std::is_same<T, WindowEvent>::value) {
				p_listeners = &m_window_event_listeners;
			}


			// Iterate over listeners and call callbacks
			for (int i = 0; i < p_listeners->size(); i++) {
				(*p_listeners)[i]->OnEvent(t_event);
			}

		}

		std::vector<const EventListener<EngineCoreEvent>*> m_engine_core_listeners;
		std::vector<const EventListener<WindowEvent>*> m_window_event_listeners;

	};

}