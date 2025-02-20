#pragma once
#include "rendering/renderpasses/Renderpass.h"
#include "rendering/Textures.h"
#include "shaders/Shader.h"
#include "framebuffers/Framebuffer.h"

namespace ORNG {
	class TransparencyPass : public Renderpass {
	public:
		TransparencyPass(class RenderGraph* p_graph) : Renderpass(p_graph, "Transparency") {};

		void Init() override;

		void DoPass() override;

		void Destroy() override;
	private:
		Texture2D m_transparency_accum{ "" };
		Texture2D m_transparency_revealage{""};

		Texture2D* mp_depth_tex = nullptr;

		class Scene* mp_scene = nullptr;

		ShaderVariants m_transparency_shader_variants;
		Shader m_transparency_composite_shader;
		Framebuffer m_transparency_fb;
		Framebuffer m_composition_fb;
	};
}