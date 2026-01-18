#include "pch/pch.h"
#include "events/EventManager.h"

#include "util/Log.h"

namespace ORNG {
	void Logger::Init() {
		m_log_file = new std::ofstream("log.txt", std::ios::trunc);
		m_logs = new std::deque<LogEvent>();
	}

	void Logger::InitFrom(std::deque<LogEvent>* logs, std::ofstream* log_file) {
		m_logs = logs;
		m_log_file = log_file;
	}

	std::string Logger::GetLastLog() {
		return m_logs->empty() ? "" : (*m_logs)[m_logs->size() - 1].content;
	}

	void Logger::OnLog(LogType type) {
		Events::EventManager::DispatchEvent(LogEvent{type, GetLastLog()});
	}

	inline std::string GetTimestamp() {
		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);

		std::ostringstream ss;
		ss << std::put_time(std::localtime(&in_time_t), "%H:%M:%S");
		return ss.str();
	}

	const char* Logger::GetLogColour(LogType type) {
		switch(type) {
			case LogType::L_TRACE:    return "\033[0m"; // default
			case LogType::L_INFO:     return "\033[92m"; // green
			case LogType::L_WARN:     return "\033[93m"; // yellow
			case LogType::L_ERROR:    return "\033[91m"; // red
			case LogType::L_CRITICAL: return "\033[41;97m"; // red background, white text
			default:                  return "\033[0m";  // reset
		}
	}

	std::string Logger::GetLogFormatStr(LogType type) {
		std::stringstream ss;
		ss << '[' << GetTimestamp() << "][CORE]";

		switch (type) {
			case LogType::L_TRACE:
				ss << "[TRC]";
				break;
			case LogType::L_INFO:
				ss << "[INF]";
				break;
			case LogType::L_WARN:
				ss << "[WRN]";
				break;
			case LogType::L_ERROR:
				ss << "[ERR]";
				break;
			case LogType::L_CRITICAL:
				ss << "[CRT]";
				break;
		}

		ss << " ";

		return ss.str();
	}

}
