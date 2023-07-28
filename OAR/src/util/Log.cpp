#include "pch/pch.h"

#include "util/Log.h"
#include "util/util.h"

namespace ORNG {

	std::shared_ptr<spdlog::logger> Log::s_core_logger;

	void Log::Init() {
		spdlog::set_pattern("%^[%T] %n: %v%$");
		s_core_logger = spdlog::stdout_color_mt("ORNG");
		s_core_logger->set_level(spdlog::level::trace);
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