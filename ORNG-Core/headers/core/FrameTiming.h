#pragma once
#include "util/TimeStep.h"

namespace ORNG {
	class FrameTiming {
	public:
		static void Init(FrameTiming* p_instance = nullptr) {
			mp_instance = p_instance ? p_instance : new FrameTiming();
		}

		static FrameTiming& Get() {
			DEBUG_ASSERT(mp_instance);
			return *mp_instance;
		}

		static float GetTimeStep() {
			return Get().IGetTimeStep();
		}

		static float GetTotalElapsedTime() {
			return Get().total_elapsed_time;
		}

		static unsigned GetFrameCount() {
			return Get().current_frame;
		}

		static void Update() {
			Get().IUpdate();
		}

	private:
		void IUpdate() {
			current_frame++;

			last_frame_time = current_frame_time;
			current_frame_time = std::chrono::steady_clock::now();
			current_frame_time_step = std::chrono::duration_cast<std::chrono::microseconds>(current_frame_time - last_frame_time).count() / 1000.0;
			total_elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - m_application_start_time).count() / 1000.0;
		}

		float IGetTimeStep() const {
			return current_frame_time_step;
		}

		inline static FrameTiming* mp_instance = nullptr;

		unsigned current_frame = 0;
		double current_frame_time_step = 0;
		double total_elapsed_time = 0;
		std::chrono::steady_clock::time_point last_frame_time = std::chrono::steady_clock::now();
		std::chrono::steady_clock::time_point current_frame_time = std::chrono::steady_clock::now();

		inline const static std::chrono::steady_clock::time_point m_application_start_time = std::chrono::steady_clock::now();
	};
}