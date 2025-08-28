#pragma once
#include "util/TimeStep.h"
#include "util/util.h"

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
			return static_cast<float>(Get().IGetTimeStep());
		}

		static double GetTimeStepHighPrecision() {
			return Get().IGetTimeStep();
		}

		static float GetTotalElapsedTime() {
			return static_cast<float>(Get().total_elapsed_time);
		}

		static double GetTotalElapsedTimeHighPrecision() {
			return Get().total_elapsed_time;
		}

		static size_t GetFrameCount() {
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
			current_frame_time_step = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(current_frame_time - last_frame_time).count()) / 1000.0;
			total_elapsed_time = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - m_application_start_time).count()) / 1000.0;
		}

		double IGetTimeStep() const {
			return current_frame_time_step;
		}

		inline static FrameTiming* mp_instance = nullptr;

		size_t current_frame = 0;
		double current_frame_time_step = 0;
		double total_elapsed_time = 0;
		std::chrono::steady_clock::time_point last_frame_time = std::chrono::steady_clock::now();
		std::chrono::steady_clock::time_point current_frame_time = std::chrono::steady_clock::now();

		inline const static std::chrono::steady_clock::time_point m_application_start_time = std::chrono::steady_clock::now();
	};
}
