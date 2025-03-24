#pragma once
#include "rendering/Textures.h"
#include "rendering/renderpasses/Renderpass.h"
#include "shaders/Shader.h"

namespace ORNG {
	// Class which creates an SSAO texture from an input depth and normal texture
	// Currently relies on common_ubo to be set so should be used as part of the SceneRenderer (want to change this later)
	class SSAOPass : public Renderpass {
	public:
		SSAOPass(class RenderGraph* p_graph) : Renderpass(p_graph, "SSAO") {};

		void Init() override;

		void DoPass() override;

		void Destroy() override {};

		inline const Texture2D& GetSSAOTex() noexcept {
			return m_ao_tex;
		}

	private:
		Texture2D* mp_depth_tex;
		Texture2D* mp_normal_tex;

		Texture2D m_ao_tex{""};
		Shader m_ssao_shader;
	};
}