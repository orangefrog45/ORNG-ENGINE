#pragma once
#include "rendering/renderpasses/Renderpass.h"
#include "rendering/Textures.h"

namespace ORNG {
	class TransparencyPass : public Renderpass {
	public:
		TransparencyPass(class RenderGraph* p_graph) : Renderpass(p_graph, "Transparency") {};

		void Init() override;

		void DoPass() override;

		void Destroy() override;
	private:
		FullscreenTexture2D m_transparency_accum;
		FullscreenTexture2D m_transparency_revealage;

		Texture2D* mp_depth_tex = nullptr;

		class Scene* mp_scene = nullptr;

		class ShaderVariants* mp_transparency_shader_variants = nullptr;
		class Shader* mp_transparency_composite_shader = nullptr;
		class Framebuffer* mp_transparency_fb = nullptr;
		Framebuffer* mp_composition_fb = nullptr;
	};
}