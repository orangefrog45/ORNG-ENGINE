#include "pch/pch.h"
#include "core/GLStateManager.h"
#include "core/Window.h"
#include "rendering/renderpasses/BloomPass.h"
#include "rendering/Renderer.h"
#include "rendering/RenderGraph.h"
#include "shaders/Shader.h"

using namespace ORNG;

void BloomPass::Init() {
	auto& shader_library = Renderer::GetShaderLibrary();

	mp_bloom_downsample_shader = &shader_library.CreateShader("Bloom downsample");
	mp_bloom_downsample_shader->AddStage(GL_COMPUTE_SHADER, "res/core-res/shaders/BloomDownsampleCS.glsl");
	mp_bloom_downsample_shader->Init();
	mp_bloom_downsample_shader->AddUniform("u_mip_level");

	mp_bloom_upsample_shader = &shader_library.CreateShader("Bloom upsample");
	mp_bloom_upsample_shader->AddStage(GL_COMPUTE_SHADER, "res/core-res/shaders/BloomUpsampleCS.glsl");
	mp_bloom_upsample_shader->Init();
	mp_bloom_upsample_shader->AddUniform("u_mip_level");

	mp_bloom_threshold_shader = &shader_library.CreateShader("Bloom threshold");
	mp_bloom_threshold_shader->AddStage(GL_COMPUTE_SHADER, "res/core-res/shaders/BloomThresholdCS.glsl");
	mp_bloom_threshold_shader->Init();
	mp_bloom_threshold_shader->AddUniforms("u_threshold", "u_knee");

	mp_composition_shader = &shader_library.CreateShader("Bloom composition");
	mp_composition_shader->AddStage(GL_COMPUTE_SHADER, "res/core-res/shaders/BloomCompositeCS.glsl");
	mp_composition_shader->Init();
	mp_composition_shader->AddUniforms("u_bloom_intensity");
}


void BloomPass::DoPass() {
	const Bloom& bloom_settings = mp_graph->GetData<PostProcessingSettings>("PPS")->bloom;
	auto* p_output = mp_graph->GetData<Texture2D>("OutCol");
	auto* p_input = mp_graph->GetData<Texture2D>("BloomInCol");

	const auto& spec = p_input->GetSpec();
	unsigned width = spec.width;
	unsigned height = spec.height;

	GL_StateManager::BindTexture(GL_TEXTURE_2D, p_input->GetTextureHandle(), GL_TEXTURE1);
	mp_bloom_threshold_shader->ActivateProgram();
	mp_bloom_threshold_shader->SetUniform("u_threshold", bloom_settings.threshold);
	mp_bloom_threshold_shader->SetUniform("u_knee", bloom_settings.knee);
	// Isolate bright spots
	glBindImageTexture(0, m_bloom_tex.GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glDispatchCompute((GLuint)glm::ceil((float)width / 16.f), (GLuint)glm::ceil((float)height / 16.f), 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	// Downsample passes
	GL_StateManager::BindTexture(GL_TEXTURE_2D, m_bloom_tex.GetTextureHandle(), GL_StateManager::TextureUnits::BLOOM, true);
	const int max_mip_layer = 6;
	mp_bloom_downsample_shader->ActivateProgram();
	for (int i = 1; i < max_mip_layer + 1; i++) {
		mp_bloom_downsample_shader->SetUniform("u_mip_level", i);
		glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, m_bloom_tex.GetTextureHandle(), i, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
		glDispatchCompute((GLuint)glm::ceil(((float)width / 32.f) / (float)i), (GLuint)glm::ceil(((float)height / 32.f) / (float)i), 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	// Upsample passes
	mp_bloom_upsample_shader->ActivateProgram();
	for (int i = max_mip_layer - 1; i >= 0; i--) {
		mp_bloom_upsample_shader->SetUniform("u_mip_level", i);
		glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, m_bloom_tex.GetTextureHandle(), i, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
		glDispatchCompute((GLuint)glm::ceil(((float)width / 16.f) / (float)(i + 1)), (GLuint)glm::ceil(((float)height / 16.f) / (float)(i + 1)), 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	mp_composition_shader->ActivateProgram();
	mp_composition_shader->SetUniform("u_bloom_intensity", bloom_settings.intensity);
	glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, p_output->GetTextureHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
	glDispatchCompute((GLuint)glm::ceil((float)width / 8.f), (GLuint)glm::ceil((float)height / 8.f), 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
