#pragma once
#include "rendering/Textures.h"

namespace ORNG {
	class Framebuffer;
	class Shader;


	class Skybox {
	public:
		friend class Renderer;
		void Init();
		const TextureCubemap& GetCubeMapTexture() const;
	private:
		unsigned int m_resolution = 4096;
		Framebuffer* mp_hdr_converter_fb = nullptr;
		Shader* mp_hdr_converter_shader = nullptr;
	};
}