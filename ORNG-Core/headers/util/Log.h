#pragma once
#include "../extern/spdlog/include/spdlog/spdlog.h"
#include "../extern/spdlog/include/spdlog/sinks/stdout_color_sinks.h"

namespace ORNG {

	class Log {
	public:

		static void Init();
		static std::string GetLastLog();
		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_core_logger; }
		static void GLLogMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void*);
	private:
		static std::shared_ptr<spdlog::logger> s_core_logger;
	};

}

#define ORNG_CORE_TRACE(...) ORNG::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define ORNG_CORE_INFO(...) ORNG::Log::GetCoreLogger()->info(__VA_ARGS__)
#define ORNG_CORE_WARN(...) ORNG::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define ORNG_CORE_ERROR(...) ORNG::Log::GetCoreLogger()->error(__VA_ARGS__)
#define ORNG_CORE_CRITICAL(...) ORNG::Log::GetCoreLogger()->critical(__VA_ARGS__)