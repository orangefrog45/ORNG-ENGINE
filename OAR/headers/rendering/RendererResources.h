#pragma once
#include <memory>
#include "Texture2D.h"
#include "glew.h"
#include "glfw3.h"
#include "util/util.h"

/* Resource/data pool for anything rendering related */
class RendererResources {
	friend class Renderer;
public:
	RendererResources(const RendererResources&) = delete;
	static void BindTexture(int target, int texture, int tex_unit);
	static auto& GetMissingTexture() { ASSERT(Get().m_missing_texture->GetTextureRef() != -1); return Get().m_missing_texture; }

	static int GetWindowWidth() { return Get().WINDOW_WIDTH; };
	static int GetWindowHeight() { return Get().WINDOW_HEIGHT; };
	static void Init() { Get().IInit(); };

	static RendererResources& Get() {
		static RendererResources s_instance;
		return s_instance;
	}

	static const unsigned int max_point_lights = 8;
	static const unsigned int max_spot_lights = 8;

	struct UniformBindingPoints { // these layouts are set manually in shaders, remember to change those with these
		static const unsigned int PVMATRICES = 0;
		static const unsigned int POINT_LIGHTS = 1;
		static const unsigned int SPOT_LIGHTS = 2;
	};

	struct TextureUnits {
		static const unsigned int COLOR = GL_TEXTURE1;
		static const unsigned int SPECULAR = GL_TEXTURE2;

		static const unsigned int DIR_SHADOW_MAP = GL_TEXTURE3;
		static const unsigned int SPOT_SHADOW_MAP = GL_TEXTURE4;
		static const unsigned int POINT_SHADOW_MAP = GL_TEXTURE5;
		static const unsigned int WORLD_POSITIONS = GL_TEXTURE6;
		static const unsigned int NORMAL_MAP = GL_TEXTURE7;
		static const unsigned int TERRAIN_DIFFUSE = GL_TEXTURE8;

	};

	struct TextureUnitIndexes {
		static const unsigned int COLOR = 1;
		static const unsigned int SPECULAR = 2;
		static const unsigned int DIR_SHADOW_MAP = 3;
		static const unsigned int SPOT_SHADOW_MAP = 4;
		static const unsigned int POINT_SHADOW_MAP = 5;
		static const unsigned int WORLD_POSITIONS = 6;
		static const unsigned int NORMAL_MAP = 7;
		static const unsigned int TERRAIN_DIFFUSE = 8;
	};
private:
	int WINDOW_WIDTH = 1920;
	int WINDOW_HEIGHT = 1080;
	void IInit();
	RendererResources() = default;
	inline static std::unique_ptr<Texture2D> m_missing_texture;
};

