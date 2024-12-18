#include "pch/pch.h"
#include "core/GLStateManager.h"
#include "util/Log.h"

namespace ORNG {
	void GL_StateManager::IBindTexture(int target, int texture, int tex_unit, bool force_mode) {
		auto& tex_data = m_current_texture_bindings[tex_unit];

		if (!force_mode && tex_data.tex_obj == texture && tex_data.tex_target == target)
			return;

		tex_data.tex_obj = texture;
		tex_data.tex_target = target;

		glActiveTexture(tex_unit);
		glBindTexture(target, texture);
	}

	void GLAPIENTRY GL_LogMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void*) {
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
			ORNG_CORE_INFO("[OPENGL DEBUG LOW] {0}", message);
			break;
		}
	}

	void GL_StateManager::I_InitGL() {
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
		glCullFace(GL_BACK);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glLineWidth(3.f);
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

		glDebugMessageCallback(GL_LogMessage, nullptr);
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	}
}