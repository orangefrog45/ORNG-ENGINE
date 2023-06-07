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
		inline static void RegisterListener(const EventListener<T>& listener) {
			Get().IRegisterListener(listener);
		};

	private:
		static EventManager& Get() {
			static EventManager s_instance;
			return s_instance;
		}

		template <std::derived_from<Event> T>
		void IRegisterListener(const EventListener<T>& listener) {

			if (!listener.OnEvent) {
				OAR_CORE_ERROR("Failed registering listener, OnEvent callback is nullptr");
				return;
			}

			bool is_registered = false;

			if constexpr (std::is_same<T, EngineCoreEvent>::value) {
				m_engine_core_listeners.push_back(&listener);
				is_registered = true;
			}

			if (!is_registered) {
				OAR_CORE_ERROR("Failed registering listener, unsupported event type.");
			}

		};



		template <std::derived_from<Event> T>
		void IDispatchEvent(const T& t_event) {

			if constexpr (std::is_same<T, EngineCoreEvent>::value) {
				for (int i = 0; i < m_engine_core_listeners.size(); i++) {

					// Delete any inactive listeners
					if (!m_engine_core_listeners[i]->OnEvent) {
						m_engine_core_listeners.erase(m_engine_core_listeners.begin() + i);
						return;
					}

					m_engine_core_listeners[i]->OnEvent(t_event);
				}
			}

		}

		std::vector<const EventListener<EngineCoreEvent>*> m_engine_core_listeners;

	};

}