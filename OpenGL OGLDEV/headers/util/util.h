#pragma once
#include <glew.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include "GLErrorHandling.h"

#define ASSERT(x) if (!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
    x;\
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))

#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))

namespace PrintUtils {
	std::string GetFormattedTime();
	void PrintWarning(const std::string& text);
	void PrintDebug(const std::string& text);
	void PrintSuccess(const std::string& text);
	void PrintError(const std::string& text);
}

namespace TextureUnits {
	constexpr int COLOR_TEXTURE_UNIT = GL_TEXTURE0;
	constexpr int SPECULAR_TEXTURE_UNIT = GL_TEXTURE1;
}

#define BASIC_VERTEX 0
#define BASIC_FRAGMENT 1

#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices)