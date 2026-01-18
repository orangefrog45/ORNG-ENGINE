#pragma once
#include "util/util.h"

namespace ORNG {
	class ProfilingTimers {
	public:
		// String contains function name and the associated timing for it
		static void StoreTimerData(const std::string& string) {
			Get().m_timer_data.push_back(string);
		}

		// Clears all timer data, should be called each frame
		static void UpdateTimers(double timestep) {
			static double cooldown = 0;
			cooldown -= glm::min(timestep, cooldown);

			if (cooldown > 0.01) {
				Get().m_timers_should_update = false;
				return;
			}

			Get().m_timer_data.clear();
			Get().m_timers_should_update = true;
			cooldown += 500.0;
		}

		static const auto& GetTimerData() {
			return Get().m_timer_data;
		}

		static bool AreTimersEnabled() {
			return Get().m_timers_enabled;
		}

		static void SetTimersEnabled(bool enabled) {
			Get().m_timers_enabled = enabled;
		}

		static bool AreTimersReadyToUpdate() {
			return Get().m_timers_should_update;
		}
	private:
		static ProfilingTimers& Get() {
			static ProfilingTimers s_instance;
			return s_instance;
		}

		bool m_timers_enabled = false;
		bool m_timers_should_update = true;
		std::vector<std::string> m_timer_data;
	};

	struct TimerObject {
		explicit TimerObject(const char* name) : func_name(name) {}
		TimerObject(const TimerObject&) = delete;
		~TimerObject() {
			if (ProfilingTimers::AreTimersEnabled() && ProfilingTimers::AreTimersReadyToUpdate()) {
				double time_elapsed = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - first_time).count());
				ProfilingTimers::StoreTimerData(std::format("{} - {}ms", func_name, time_elapsed / 1000.0));
			}
		}

		std::chrono::steady_clock::time_point first_time = std::chrono::steady_clock::now();
		const char* func_name;
	};

	struct GPU_TimerObject {
		explicit GPU_TimerObject(const char* name) : func_name(name) {
			if (ProfilingTimers::AreTimersEnabled() && ProfilingTimers::AreTimersReadyToUpdate()) {
				glGenQueries(1, &query);
				glBeginQuery(GL_TIME_ELAPSED, query);
			}
		}

		void ForceStartQuery() {
			glGenQueries(1, &query);
			glBeginQuery(GL_TIME_ELAPSED, query);
		}

		unsigned GetTimeElapsed() {
			unsigned int result;
			glEndQuery(GL_TIME_ELAPSED);
			glGetQueryObjectuiv(query, GL_QUERY_RESULT, &result);
			query_finished = true;
			return static_cast<unsigned>(result / 1000000.0);
		}

		GPU_TimerObject(const GPU_TimerObject&) = delete;
		~GPU_TimerObject() {
			if (ProfilingTimers::AreTimersEnabled() && ProfilingTimers::AreTimersReadyToUpdate() && !query_finished) {
				unsigned int result;
				glEndQuery(GL_TIME_ELAPSED);
				glGetQueryObjectuiv(query, GL_QUERY_RESULT, &result);
				ProfilingTimers::StoreTimerData(std::format("{} - {}ms", func_name, result / 1000000.0));
			}
		}

		GLuint query;
		std::chrono::steady_clock::time_point first_time = std::chrono::steady_clock::now();
		const char* func_name;
		bool query_finished = false;
	};

#define ORNG_ENABLE_PROFILING

#ifdef ORNG_ENABLE_PROFILING
#define ORNG_PROFILE_FUNC() \
		TimerObject CONCAT(timerObject_, __LINE__) { FUNC_NAME }
#define ORNG_PROFILE_FUNC_GPU() \
		GPU_TimerObject CONCAT(timerObject_, __LINE__) { FUNC_NAME }
#else
#define ORNG_PROFILE_FUNC_GPU()
#define ORNG_PROFILE_FUNC()
#endif

}
