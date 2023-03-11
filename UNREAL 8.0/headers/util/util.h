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

namespace PrintUtils {
	std::string GetFormattedTime();
	void PrintWarning(const std::string& text);
	void PrintDebug(const std::string& text);
	void PrintSuccess(const std::string& text);
	void PrintError(const std::string& text);
	std::string RoundDouble(double value);
}

namespace RenderData {
	constexpr int WINDOW_WIDTH = 1920;
	constexpr int WINDOW_HEIGHT = 1080;
}

namespace UniformBindingPoints { // these layouts are set manually in shaders, remember to change those with these
	constexpr unsigned int PVMATRICES = 0;
	constexpr unsigned int POINT_LIGHTS = 1;
	constexpr unsigned int SPOT_LIGHTS = 2;
}

namespace TextureUnits {
	constexpr int COLOR_TEXTURE_UNIT = GL_TEXTURE1;
	constexpr int SPECULAR_TEXTURE_UNIT = GL_TEXTURE2;
	constexpr int SHADOW_MAP_TEXTURE_UNIT = GL_TEXTURE3;
}

namespace TextureUnitIndexes {
	constexpr int COLOR_TEXTURE_UNIT_INDEX = 1;
	constexpr int SPECULAR_TEXTURE_UNIT_INDEX = 2;
	constexpr int SHADOW_MAP_TEXTURE_UNIT_INDEX = 3;
}

enum class MeshShaderMode {
	LIGHTING = 0,
	FLAT_COLOR = 1
};

#define BASIC_VERTEX 0
#define BASIC_FRAGMENT 1

#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices)