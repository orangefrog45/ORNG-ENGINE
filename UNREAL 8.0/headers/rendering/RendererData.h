#pragma once
namespace RendererData {
	constexpr int WINDOW_WIDTH = 1920;
	constexpr int WINDOW_HEIGHT = 1080;
	static const unsigned int max_point_lights = 8;
	static const unsigned int max_spot_lights = 8;


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

}


enum class MeshShaderMode {
	LIGHTING = 0,
	FLAT_COLOR = 1,
	REFLECT = 2
};