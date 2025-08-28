#include "pch/pch.h"

#include "util/TimeStep.h"

using namespace ORNG;

long long TimeStep::GetTimeInterval() const {
	switch (m_units) {
	case TimeUnits::MILLISECONDS:
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_last_time).count();
	case TimeUnits::SECONDS:
		return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - m_last_time).count();
	case TimeUnits::MICROSECONDS:
		return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - m_last_time).count();
	case TimeUnits::NANOSECONDS:
		return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - m_last_time).count();
	}
}
