#include "pch/pch.h"
#include "events/EventManager.h"


#include "util/Log.h"


namespace ORNG {
	std::shared_ptr<spdlog::logger> Log::s_core_logger = nullptr;
	std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> Log::s_ringbuffer_sink = nullptr;
	std::shared_ptr<spdlog::sinks::basic_file_sink_mt> Log::s_file_sink = nullptr;

	// How many logged messages are saved and stored in memory that can be retrieved by the program
	constexpr int MAX_LOG_HISTORY = 20;

	void Log::Init() {
		// Clear log file
		std::ofstream("log.txt", std::ios::trunc);

		s_ringbuffer_sink = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(MAX_LOG_HISTORY);
		s_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("log.txt");

		std::vector<spdlog::sink_ptr> sinks;
		s_ringbuffer_sink->set_pattern("%^[%T] %n: %v%$");
		sinks.push_back(s_ringbuffer_sink);
		sinks.push_back(s_file_sink);

		auto color_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		color_sink->set_pattern("%^[%T] %n: %v%$");
		sinks.push_back(color_sink);

		s_core_logger = std::make_shared<spdlog::logger>("ORNG", sinks.begin(), sinks.end());
		s_core_logger->set_level(spdlog::level::trace);
	}

	std::string Log::GetLastLog() {
		auto last_formatted = s_ringbuffer_sink->last_formatted(1);
		return last_formatted.empty() ? "" : last_formatted[0];
	}

	std::vector<std::string> Log::GetLastLogs() {
		return s_ringbuffer_sink->last_formatted(MAX_LOG_HISTORY);
	}
	
	void Log::Flush() {
		GetCoreLogger()->flush();
	}

	std::shared_ptr<spdlog::logger>& Log::GetCoreLogger() {
		return s_core_logger;
	}

	void Log::OnLog(LogEvent::Type type)
	{
		Events::EventManager::DispatchEvent(LogEvent{type, GetLastLog() });
	}
}
