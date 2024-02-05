#include "pch/pch.h"
#include "spdlog/sinks/ringbuffer_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#include "util/Log.h"


namespace ORNG {
	// How many logged messages are saved and stored in memory that can be retrieved by the program
	constexpr int MAX_LOG_HISTORY = 20;

	static auto s_ringbuffer_sink = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(MAX_LOG_HISTORY);
	static auto s_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("log.txt");
	std::shared_ptr<spdlog::logger> Log::s_core_logger;

	void Log::Init() {
		// Clear log file
		std::ofstream("log.txt", std::ios::trunc);

		std::vector<spdlog::sink_ptr> sinks;
		s_ringbuffer_sink->set_pattern("% %v%$");
		sinks.push_back(s_ringbuffer_sink);
		sinks.push_back(s_file_sink);

		auto color_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		color_sink->set_pattern("%^[%T] %n: %v%$");
		sinks.push_back(color_sink);

		s_core_logger = std::make_shared<spdlog::logger>("ORNG", sinks.begin(), sinks.end());
		s_core_logger->set_level(spdlog::level::trace);
	}

	std::string Log::GetLastLog() {
		return s_ringbuffer_sink->last_formatted()[4];
	}

	std::vector<std::string> Log::GetLastLogs() {
		return std::move(s_ringbuffer_sink->last_formatted());
	}

	void Log::Flush() {
		GetCoreLogger()->flush();
	}


	std::shared_ptr<spdlog::logger>& Log::GetCoreLogger() {
		return s_core_logger;
	}


}