#pragma once
#include "rendering/renderpasses/Renderpass.h"
#include "rendering/Textures.h"

namespace ORNG {
	class PostProcessPass : public Renderpass {
	public:
		PostProcessPass(class RenderGraph* p_graph) : Renderpass(p_graph, "Post process") {};

		void Init() override;

		void DoPass() override;

		void Destroy() override;
	private:
		Texture2D* mp_fog_tex = nullptr;

		class Shader* mp_post_process_shader = nullptr;
		class Scene* mp_scene = nullptr;
	};
}