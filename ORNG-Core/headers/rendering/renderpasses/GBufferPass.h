#pragma once
#include "Renderpass.h"
#include "rendering/Textures.h"
#include "shaders/Shader.h"
#include "framebuffers/Framebuffer.h"

namespace ORNG {
	class GBufferPass : public Renderpass {
	public:
		GBufferPass(class RenderGraph* p_graph) : Renderpass(p_graph, "Gbuffer") {};

		void Init() override;

		void DoPass() override;

		void Destroy() override;

		Texture2D normals{ "" };
		Texture2D albedo{""};
		Texture2D rma{""};
		Texture2D depth{""};
		Texture2D shader_ids{""};

		ShaderVariants sv;
		ShaderVariants displacement_sv;

		Framebuffer framebuffer;

		class Scene* mp_scene = nullptr;
	};
}