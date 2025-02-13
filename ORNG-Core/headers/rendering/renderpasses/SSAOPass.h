#pragma once
#include "rendering/Textures.h"

namespace ORNG {
	// Class which creates an SSAO texture from an input depth and normal texture
	// Currently relies on common_ubo to be set so should be used as part of the SceneRenderer (want to change this later)
	class SSAOPass {
	public:
		void Init();
		void DoPass(Texture2D& depth_tex, Texture2D& normal_tex);

		inline const FullscreenTexture2D& GetSSAOTex() noexcept {
			return m_ao_tex;
		}

	private:
		FullscreenTexture2D m_ao_tex;
		class Shader* mp_ssao_shader = nullptr;
	};
}