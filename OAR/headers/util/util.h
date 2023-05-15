#pragma once
namespace ORNG {

#define BREAKPOINT __debugbreak()
#define ASSERT(x) if (!(x)) __debugbreak()

#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))

	constexpr bool SHADER_DEBUG_MODE = false; // true = CreateUniform throws error if uniform doesn't exist
}

