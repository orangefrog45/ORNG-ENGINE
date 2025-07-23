#pragma once
#include "Renderpass.h"
#include "rendering/Textures.h"
#include "rendering/Material.h"
#include "shaders/Shader.h"
#include "framebuffers/Framebuffer.h"

namespace ORNG {
	class DepthPass : public Renderpass {
		friend class LightingPass;
	public:
		DepthPass(class RenderGraph* p_graph) : Renderpass(p_graph, "Depth") {};

		void Init() override;

		void DoPass() override;

		void Destroy() override;

		void DrawAllMeshesDepth(RenderGroup render_group, Scene* p_scene);

		Texture2DArray directional_light_depth_tex{ "SR Directional depth array" };

		ShaderVariants sv;
		Framebuffer fb;
		class Scene* p_scene = nullptr;
		class SpotlightSystem* p_spotlight_system = nullptr;
		class PointlightSystem* p_pointlight_system = nullptr;
	};
}