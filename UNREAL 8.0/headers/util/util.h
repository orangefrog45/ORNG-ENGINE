#pragma once
#include "GLErrorHandling.h"

#define BREAKPOINT __debugbreak()
#define ASSERT(x) if (!(x)) __debugbreak()
#define GLCall(x) GLClearError();\
    x;\
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))

#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))

constexpr bool SHADER_DEBUG_MODE = false; // true = GetUniform throws error if uniform doesn't exist

