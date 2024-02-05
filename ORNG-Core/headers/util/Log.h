#pragma once
#ifndef LOG_H
#define LOG_H


#include <spdlog/logger.h>
#include "spdlog/sinks/stdout_color_sinks.h"


namespace ORNG {

	class Log {
	public:

		static void Init();
		static void Flush();
		static std::string GetLastLog();
		static std::vector<std::string> GetLastLogs();
		static std::shared_ptr<spdlog::logger>& GetCoreLogger();
	private:
		static std::shared_ptr<spdlog::logger> s_core_logger;
	};

}

#define ORNG_CORE_TRACE(...) ORNG::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define ORNG_CORE_INFO(...) ORNG::Log::GetCoreLogger()->info(__VA_ARGS__)
#define ORNG_CORE_WARN(...) ORNG::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define ORNG_CORE_ERROR(...) ORNG::Log::GetCoreLogger()->error(__VA_ARGS__)
#define ORNG_CORE_CRITICAL(...) ORNG::Log::GetCoreLogger()->critical(__VA_ARGS__)

#endif