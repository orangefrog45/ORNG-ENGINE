#pragma once
#ifndef LOG_H
#define LOG_H

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include <spdlog/logger.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "events/Events.h"

template<typename T>
std::string Format(const T&);

template<>
inline std::string Format<glm::vec4>(const glm::vec4& v) {
	return std::format("[{}, {}, {}, {}]", v.x, v.y, v.z, v.w);
}

template<>
inline std::string Format<glm::vec3>(const glm::vec3& v) {
	return std::format("[{}, {}, {}]", v.x, v.y, v.z);
}

template<>
inline std::string Format<glm::vec2>(const glm::vec2& v) {
	return std::format("[{}, {}]", v.x, v.y);
}

namespace ORNG {
	struct LogEvent : public Events::Event {
		enum class Type {
			L_TRACE,
			L_INFO,
			L_WARN,
			L_ERROR,
			L_CRITICAL
		} type;

		LogEvent(Type _type, const std::string& _content) : type(_type), content(_content) {}
		const std::string& content;
	};

	class Log { 
	public:
		static void Init();
		static void InitFrom(std::shared_ptr<spdlog::logger> p_core_logger, std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> p_ringbuffer_sink) {
			s_core_logger = p_core_logger;
			s_ringbuffer_sink = p_ringbuffer_sink;
		}

		static void Flush();
		static std::string GetLastLog();
		static std::vector<std::string> GetLastLogs();
		static std::shared_ptr<spdlog::logger>& GetCoreLogger();
		static auto& GetRingbufferSink() { return s_ringbuffer_sink; }
		static void OnLog(LogEvent::Type type);
	private:
		static std::shared_ptr<spdlog::logger> s_core_logger;
		static std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> s_ringbuffer_sink;
		static std::shared_ptr<spdlog::sinks::basic_file_sink_mt> s_file_sink;
	};

}


#define ORNG_CORE_TRACE(...) do {ORNG::Log::GetCoreLogger()->trace(__VA_ARGS__); ORNG::Log::OnLog(ORNG::LogEvent::Type::L_TRACE); } while(false)
#define ORNG_CORE_INFO(...) do {ORNG::Log::GetCoreLogger()->info(__VA_ARGS__); ORNG::Log::OnLog(ORNG::LogEvent::Type::L_INFO); } while(false)
#define ORNG_CORE_WARN(...) do {ORNG::Log::GetCoreLogger()->warn(__VA_ARGS__); ORNG::Log::OnLog(ORNG::LogEvent::Type::L_WARN); } while(false)
#define ORNG_CORE_ERROR(...) do {ORNG::Log::GetCoreLogger()->error(__VA_ARGS__); ORNG::Log::OnLog(ORNG::LogEvent::Type::L_ERROR); } while(false)
#define ORNG_CORE_CRITICAL(...) do {ORNG::Log::GetCoreLogger()->critical(__VA_ARGS__); ORNG::Log::Flush(); ORNG::Log::OnLog(ORNG::LogEvent::Type::L_CRITICAL); } while(false)

#endif
