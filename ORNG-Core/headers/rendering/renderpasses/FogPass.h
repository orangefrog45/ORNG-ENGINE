#pragma once
#include "rendering/renderpasses/Renderpass.h"
#include "rendering/Textures.h"
#include "shaders/Shader.h"

namespace ORNG {
	class FogPass : public Renderpass {
	public:
		FogPass(class RenderGraph* p_graph) : Renderpass(p_graph, "Fog") {};

		void Init() override;

		void DoPass() override;

		void Destroy() override;

		Texture2D& GetFinalFogTex() {
			return m_fog_blur_tex_1;
		}
	private:
		Texture2D m_fog_output_tex{""};
		Texture2D m_fog_blur_tex_1{""};
		Texture2D m_fog_blur_tex_2{""};
		Texture2D* mp_depth_tex = nullptr;

		class Scene* mp_scene = nullptr;

		Shader m_fog_shader;
		Shader m_blur_shader;
		ShaderVariants m_depth_aware_upsample_sv;
	};
}