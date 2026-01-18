#pragma once
#ifndef LOG_H
#define LOG_H

#include <iostream>

#include "events/Events.h"

template<typename T>
std::string Format(const T&);

template<>
inline std::string Format<glm::vec4>(const glm::vec4& v) {
	return std::format("[{:.4f}, {:.4f}, {:.4f}, {:.4f}]", v.x, v.y, v.z, v.w);
}

template<>
inline std::string Format<glm::vec3>(const glm::vec3& v) {
	return std::format("[{:.4f}, {:.4f}, {:.4f}]", v.x, v.y, v.z);
}

template<>
inline std::string Format<glm::vec2>(const glm::vec2& v) {
	return std::format("[{:.4f}, {:.4f}]", v.x, v.y);
}

namespace ORNG {
	enum class LogType {
		L_TRACE,
		L_INFO,
		L_WARN,
		L_ERROR,
		L_CRITICAL
	};

	struct LogEvent : public Events::Event {
		LogEvent(LogType _type, const std::string& _content) : type(_type), content(_content) {}
		LogType type;
		const std::string& content;
	};

	class Logger {
	public:
		static void Init();
		static void InitFrom(std::deque<LogEvent>* logs, std::ofstream* log_file);

		static std::string GetLastLog();
		static void OnLog(LogType type);

		template<typename... Args>
		static void Log(LogType type, std::string_view fmt, Args&&... args) {
			std::string msg = std::vformat(fmt, std::make_format_args(args...));
			Log(type, msg);
		}

		template<typename T>
		static void Log(LogType type, T s) {
			std::string str = GetLogFormatStr(type) + std::format("{}", s);

			// Everything is flushed immediately for crash safety
			std::cout << GetLogColour(type) << str << "\033[0m\n" << std::flush;
			(*m_log_file) << str << "\n";

			m_logs->emplace_back(type, str);

			if (m_logs->size() > 500)
				m_logs->pop_front();

			m_log_file->flush();

			OnLog(type);
		}

		static auto GetLogs() {
			return m_logs;
		}

		static auto GetLogFile() {
			return m_log_file;
		}

	private:
		static std::string GetLogFormatStr(LogType type);
		static const char* GetLogColour(LogType type);

		inline static std::deque<LogEvent>* m_logs;
		inline static std::ofstream* m_log_file;
	};

}

#define ORNG_CORE_TRACE(...) do {ORNG::Logger::Log(ORNG::LogType::L_TRACE, __VA_ARGS__); } while(false)
#define ORNG_CORE_INFO(...) do {ORNG::Logger::Log(ORNG::LogType::L_INFO, __VA_ARGS__); } while(false)
#define ORNG_CORE_WARN(...) do {ORNG::Logger::Log(ORNG::LogType::L_WARN, __VA_ARGS__); } while(false)
#define ORNG_CORE_ERROR(...) do {ORNG::Logger::Log(ORNG::LogType::L_ERROR, __VA_ARGS__); } while(false)
#define ORNG_CORE_CRITICAL(...) do {ORNG::Logger::Log(ORNG::LogType::L_CRITICAL, __VA_ARGS__); BREAKPOINT; } while(false)

#endif
