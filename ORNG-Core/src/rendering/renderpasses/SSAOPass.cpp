#include "pch/pch.h"
#include "core/GLStateManager.h"
#include "core/Window.h"
#include "rendering/renderpasses/SSAOPass.h"
#include "rendering/renderpasses/GBufferPass.h"
#include "rendering/Renderer.h"
#include "rendering/RenderGraph.h"

using namespace ORNG;

void SSAOPass::Init() {
	ssao_shader.AddStage(GL_COMPUTE_SHADER, "res/core-res/shaders/SSAOCS.glsl");
	ssao_shader.Init();

	const auto& render_spec = mp_graph->GetData<Texture2D>("OutCol")->GetSpec();

	Texture2DSpec ssao_spec;
	ssao_spec.width = render_spec.width;
	ssao_spec.height = render_spec.height;
	ssao_spec.format = GL_RGBA;
	ssao_spec.internal_format = GL_RGBA16F;
	ssao_spec.min_filter = GL_LINEAR;
	ssao_spec.mag_filter = GL_LINEAR;
	ssao_spec.wrap_params = GL_CLAMP_TO_EDGE;
	ao_tex.SetSpec(ssao_spec);

	auto* p_gbuffer_pass = mp_graph->GetRenderpass<GBufferPass>();
	if (!p_gbuffer_pass) {
		ORNG_CORE_CRITICAL("Tried to use SSAO pass with a render graph without a GBuffer pass (which must be added)");
	}
	else {
		p_depth_tex = &p_gbuffer_pass->depth;
		p_normal_tex = &p_gbuffer_pass->normals;
	}
}

void SSAOPass::DoPass() {
	GL_StateManager::BindTexture(GL_TEXTURE_2D, p_normal_tex->GetTextureHandle(), GL_TEXTURE1);
	GL_StateManager::BindTexture(GL_TEXTURE_2D, p_depth_tex->GetTextureHandle(), GL_TEXTURE2, true);

	const auto& spec = ao_tex.GetSpec();
	ssao_shader.ActivateProgram();
	glBindImageTexture(1, ao_tex.GetTextureHandle(), 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glDispatchCompute(ceil(spec.width / 8.f), ceil(spec.height / 8.f), 1);
}
