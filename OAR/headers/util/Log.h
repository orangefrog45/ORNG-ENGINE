#pragma once
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"


class Log {
public:
	static void Init();
	inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_core_logger; }
private:
	static std::shared_ptr<spdlog::logger> s_core_logger;
};

#define OAR_CORE_TRACE(...) Log::GetCoreLogger()->trace(__VA_ARGS__)
#define OAR_CORE_INFO(...) Log::GetCoreLogger()->info(__VA_ARGS__)
#define OAR_CORE_WARN(...) Log::GetCoreLogger()->warn(__VA_ARGS__)
#define OAR_CORE_ERROR(...) Log::GetCoreLogger()->error(__VA_ARGS__)
#define OAR_CORE_CRITICAL(...) Log::GetCoreLogger()->critical(__VA_ARGS__)