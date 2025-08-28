#pragma once
#include "rendering/renderpasses/Renderpass.h"
#include "rendering/Textures.h"
#include "shaders/Shader.h"
#include "framebuffers/Framebuffer.h"

namespace ORNG {
	class TransparencyPass : public Renderpass {
	public:
		explicit TransparencyPass(class RenderGraph* p_graph) : Renderpass(p_graph, "Transparency") {}

		void Init() override;

		void DoPass() override;

		Texture2D transparency_accum{ "" };
		Texture2D transparency_revealage{""};

		Texture2D* p_depth_tex = nullptr;

		class Scene* p_scene = nullptr;

		ShaderVariants transparency_shader_variants;
		Shader transparency_composite_shader;
		Framebuffer transparency_fb;
		Framebuffer composition_fb;
	};
}
