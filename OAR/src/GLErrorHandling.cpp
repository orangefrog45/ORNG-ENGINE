#include "pch/pch.h"

#include "util/Log.h"
#include "GLErrorHandling.h"


void GLClearError() {
	while (glGetError() != GL_NO_ERROR);
}

bool GLLogCall(const char* function, const char* file, int line) {
	while (GLenum error = glGetError()) {
		OAR_CORE_CRITICAL("[OpenGL Error]: '{0}'\nFunction: '{1}'\nFile: '{2}'\nLine: '{3}'", error, function, file, line);
		return false;
	}
	return true;
}