#include "pch/pch.h"
#include "core/GLStateManager.h"
#include "core/Window.h"
#include "rendering/renderpasses/SSAOPass.h"
#include "rendering/Renderer.h"

using namespace ORNG;

void SSAOPass::Init() {
	constexpr const char* SHADER_NAME = "SSAO";
	// May already exist
	mp_ssao_shader = Renderer::GetShaderLibrary().GetShader("SSAO");

	if (!mp_ssao_shader) {
		mp_ssao_shader = &Renderer::GetShaderLibrary().CreateShader(SHADER_NAME);
		mp_ssao_shader->AddStage(GL_COMPUTE_SHADER, "res/core-res/shaders/SSAOCS.glsl");
		mp_ssao_shader->Init();
	}
		
	Texture2DSpec ssao_spec;
	ssao_spec.width = Window::GetWidth();
	ssao_spec.height = Window::GetHeight();
	ssao_spec.format = GL_RGBA;
	ssao_spec.internal_format = GL_RGBA16F;
	ssao_spec.min_filter = GL_LINEAR;
	ssao_spec.mag_filter = GL_LINEAR;
	ssao_spec.wrap_params = GL_CLAMP_TO_EDGE;
	m_ao_tex.SetSpec(ssao_spec);
}

void SSAOPass::DoPass(Texture2D& depth_tex, Texture2D& normal_tex) {
	GL_StateManager::BindTexture(GL_TEXTURE_2D, normal_tex.GetTextureHandle(), GL_TEXTURE1);
	GL_StateManager::BindTexture(GL_TEXTURE_2D, depth_tex.GetTextureHandle(), GL_TEXTURE2);

	const auto& spec = m_ao_tex.GetSpec();
	mp_ssao_shader->ActivateProgram();
	glBindImageTexture(1, m_ao_tex.GetTextureHandle(), 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glDispatchCompute(ceil(spec.width / 8.f), ceil(spec.height / 8.f), 1);
}