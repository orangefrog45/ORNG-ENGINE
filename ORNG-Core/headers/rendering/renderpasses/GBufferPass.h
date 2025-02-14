#pragma once
#include "Renderpass.h"
#include "rendering/Textures.h"

namespace ORNG {
	class GBufferPass : public Renderpass {
	public:
		GBufferPass(class RenderGraph* p_graph) : Renderpass(p_graph, "Gbuffer") {};

		void Init() override;

		void DoPass() override;

		void Destroy() override;

		FullscreenTexture2D normals;
		FullscreenTexture2D albedo;
		FullscreenTexture2D rma;
		FullscreenTexture2D depth;
		FullscreenTexture2D shader_ids;
	private:

		class ShaderVariants* mp_sv = nullptr;
		ShaderVariants* mp_displacement_sv = nullptr;

		class Framebuffer* mp_framebuffer = nullptr;

		class Scene* mp_scene = nullptr;
	};
}