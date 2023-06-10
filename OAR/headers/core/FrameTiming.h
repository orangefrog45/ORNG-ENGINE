#pragma once
#include "util/TimeStep.h"

namespace ORNG {

	class FrameTiming {

	public:
		static FrameTiming& Get() {
			static FrameTiming s_instance;
			return s_instance;
		}

		static long long GetTimeStep() {
			return Get().IGetTimeStep();
		}

		static void UpdateTimeStep() {
			Get().IUpdateTimeStep();
		}


	private:

		void IUpdateTimeStep() {
			last_frame_time = current_frame_time;
			current_frame_time = std::chrono::steady_clock::now();
			current_frame_time_step = std::chrono::duration_cast<std::chrono::microseconds>(current_frame_time - last_frame_time).count();
		}
		long long IGetTimeStep() {
			return current_frame_time_step;
		}

		long long current_frame_time_step = 0;
		std::chrono::steady_clock::time_point last_frame_time;
		std::chrono::steady_clock::time_point current_frame_time;
		TimeStep m_time_step{ TimeStep::TimeUnits::MICROSECONDS };

	};
}