#pragma once
#include "rendering/renderpasses/Renderpass.h"
#include "rendering/Textures.h"
#include "shaders/Shader.h"

namespace ORNG {
	class PostProcessPass : public Renderpass {
	public:
		PostProcessPass(class RenderGraph* p_graph) : Renderpass(p_graph, "Post process") {};

		void Init() override;

		void DoPass() override;

		void Destroy() override;

		Texture2D* p_fog_tex = nullptr;

		Shader post_process_shader;
		Scene* p_scene = nullptr;
	};
}