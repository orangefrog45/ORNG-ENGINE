#pragma once
#include <chrono>

class TimeStep {
public:
	enum class TimeUnits {
		SECONDS = 0,
		MILLISECONDS = 1,
		MICROSECONDS = 2,
		NANOSECONDS = 3
	};

	TimeStep(TimeUnits units) : m_units(units) {};

	/* Set last time to current time */
	void UpdateLastTime() { m_last_time = std::chrono::steady_clock::now(); };

	long long GetTimeInterval() const;

private:
	TimeUnits m_units;
	std::chrono::steady_clock::time_point m_last_time = std::chrono::steady_clock::now();
};