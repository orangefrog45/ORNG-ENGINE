#pragma once
#include <glew.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <cmath>
#include "GLErrorHandling.h"

#define ASSERT(x) if (!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
    x;\
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))

#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))

constexpr bool SHADER_DEBUG_MODE = false; // true = GetUniform throws error if uniform doesn't exist

namespace PrintUtils {
	std::string GetFormattedTime();
	void PrintWarning(const std::string& text);
	void PrintDebug(const std::string& text);
	void PrintSuccess(const std::string& text);
	void PrintError(const std::string& text);
	std::string RoundDouble(double value);
}

#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices)