#pragma once
#include "Renderpass.h"
#include "rendering/Textures.h"
#include "rendering/Material.h"

namespace ORNG {
	class DepthPass : public Renderpass {
		friend class LightingPass;
	public:
		DepthPass(class RenderGraph* p_graph) : Renderpass(p_graph, "Depth") {};

		void Init() override;

		void DoPass() override;

		void Destroy() override;

		void DrawAllMeshesDepth(RenderGroup render_group, Scene* p_scene);
	private:
		Texture2DArray m_directional_light_depth_tex{ "SR Directional depth array" };

		class ShaderVariants* mp_sv = nullptr;
		class Framebuffer* mp_fb = nullptr;
		class Scene* mp_scene = nullptr;
		class SpotlightSystem* mp_spotlight_system = nullptr;
		class PointlightSystem* mp_pointlight_system = nullptr;
	};
}