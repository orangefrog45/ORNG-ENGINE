#include "pch/pch.h"
#include "spdlog/sinks/ringbuffer_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "util/Log.h"
#include "util/util.h"


namespace ORNG {
	// How many logged messages are saved and stored in memory that can be retrieved by the program
	constexpr int MAX_LOG_HISTORY = 20;

	static auto s_ringbuffer_sink = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(MAX_LOG_HISTORY);
	std::shared_ptr<spdlog::logger> Log::s_core_logger;

	void Log::Init() {
		std::vector<spdlog::sink_ptr> sinks;
		s_ringbuffer_sink->set_pattern("% %v%$");
		sinks.push_back(s_ringbuffer_sink);

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

	std::shared_ptr<spdlog::logger>& Log::GetCoreLogger() {
		return s_core_logger;
	}

	void Log::GLLogMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void*) {
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:
			ORNG_CORE_CRITICAL("[OPENGL DEBUG HIGH] {0}", message);
			BREAKPOINT;
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			ORNG_CORE_WARN("[OPENGL DEBUG MEDIUM] {0}", message);
			break;
		case GL_DEBUG_SEVERITY_LOW:
			ORNG_CORE_INFO("[OPENGL DEBUG MEDIUM] {0}", message);
			break;
		}
	}
}