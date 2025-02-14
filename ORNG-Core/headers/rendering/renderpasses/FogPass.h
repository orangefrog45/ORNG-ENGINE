#pragma once
#include "rendering/renderpasses/Renderpass.h"
#include "rendering/Textures.h"

namespace ORNG {
	class FogPass : public Renderpass {
	public:
		FogPass(class RenderGraph* p_graph) : Renderpass(p_graph, "Fog") {};

		void Init() override;

		void DoPass() override;

		void Destroy() override;

		FullscreenTexture2D& GetFinalFogTex() {
			return m_fog_blur_tex_1;
		}

	private:
		FullscreenTexture2D m_fog_output_tex{ {0.5f, 0.5f} };
		FullscreenTexture2D m_fog_blur_tex_1{ {1.f, 1.f} };
		FullscreenTexture2D m_fog_blur_tex_2{ {1.f, 1.f} };
		Texture2D* mp_depth_tex = nullptr;

		class Scene* mp_scene = nullptr;

		class Shader* mp_fog_shader = nullptr;
		Shader* mp_blur_shader = nullptr;
		class ShaderVariants* mp_depth_aware_upsample_sv = nullptr;

	};
}