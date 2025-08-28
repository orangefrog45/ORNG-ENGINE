#pragma once
#include "rendering/renderpasses/Renderpass.h"
#include "rendering/Textures.h"
#include "shaders/Shader.h"

namespace ORNG {
	class FogPass : public Renderpass {
	public:
		explicit FogPass(class RenderGraph* p_graph) : Renderpass(p_graph, "Fog") {}

		void Init() override;

		void DoPass() override;

		Texture2D& GetFinalFogTex() {
			return fog_blur_tex_1;
		}
		
		Texture2D fog_output_tex{""};
		Texture2D fog_blur_tex_1{""};
		Texture2D fog_blur_tex_2{""};
		Texture2D* p_depth_tex = nullptr;

		class Scene* p_scene = nullptr;

		Shader fog_shader;
		Shader blur_shader;
		ShaderVariants depth_aware_upsample_sv;
	};
}
