#pragma once
#include "util/TimeStep.h"
#include "events/EventManager.h"

namespace ORNG {

	class FrameTiming {
	public:
		FrameTiming() {

			m_update_listener.OnEvent = [this](const Events::EngineCoreEvent& t_event) {
				if (t_event.event_type == Events::EventType::ENGINE_UPDATE)
					m_time_step.UpdateLastTime();
			};

			Events::EventManager::RegisterListener(m_update_listener);
		}

		static FrameTiming& Get() {
			static FrameTiming s_instance;
			return s_instance;
		}

		static long long GetTimeStep() {
			return Get().IGetTimeStep();
		}


	private:
		long long IGetTimeStep() {
			return m_time_step.GetTimeInterval();
		}

		TimeStep m_time_step{ TimeStep::TimeUnits::MICROSECONDS };
		Events::EventListener<Events::EngineCoreEvent> m_update_listener;

	};
}