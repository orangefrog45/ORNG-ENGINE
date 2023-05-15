#pragma once
#include "rendering/Textures.h"

namespace ORNG {

	class Skybox {
	public:
		friend class Renderer;
		void Init();
		const TextureCubemap& GetCubeMapTexture() const { return m_cubemap_texture; }

	private:
		TextureCubemap m_cubemap_texture;
	};
}