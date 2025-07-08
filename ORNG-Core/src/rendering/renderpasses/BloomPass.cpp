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

	m_bloom_downsample_shader.AddStage(GL_COMPUTE_SHADER, "res/core-res/shaders/BloomDownsampleCS.glsl");
	m_bloom_downsample_shader.Init();
	m_bloom_downsample_shader.AddUniform("u_mip_level");

	m_bloom_upsample_shader.AddStage(GL_COMPUTE_SHADER, "res/core-res/shaders/BloomUpsampleCS.glsl");
	m_bloom_upsample_shader.Init();
	m_bloom_upsample_shader.AddUniform("u_mip_level");

	m_bloom_threshold_shader.AddStage(GL_COMPUTE_SHADER, "res/core-res/shaders/BloomThresholdCS.glsl");
	m_bloom_threshold_shader.Init();
	m_bloom_threshold_shader.AddUniforms("u_threshold", "u_knee");

	m_composition_shader.AddStage(GL_COMPUTE_SHADER, "res/core-res/shaders/BloomCompositeCS.glsl");
	m_composition_shader.Init();
	m_composition_shader.AddUniforms("u_bloom_intensity");

	auto& input_spec = mp_graph->GetData<Texture2D>("BloomInCol")->GetSpec();

	Texture2DSpec bloom_spec;
	bloom_spec.format = GL_RGBA;
	bloom_spec.internal_format = GL_RGBA16F;
	bloom_spec.width = (unsigned)ceil(input_spec.width * 0.5f);
	bloom_spec.height = (unsigned)ceil(input_spec.height * 0.5f);
	bloom_spec.storage_type = GL_FLOAT;
	bloom_spec.generate_mipmaps = true;
	bloom_spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
	bloom_spec.mag_filter = GL_LINEAR;
	bloom_spec.wrap_params = GL_CLAMP_TO_EDGE;
	m_bloom_tex.SetSpec(bloom_spec);
}


void BloomPass::DoPass() {
	{
		const auto& bloom_spec = m_bloom_tex.GetSpec();
		glm::vec4 clear_val{ 0.f };
		glClearTexSubImage(m_bloom_tex.GetTextureHandle(), 0, 0, 0, 0, bloom_spec.width, bloom_spec.height, 1, GL_RGBA, GL_FLOAT, &clear_val);
	}

	const Bloom& bloom_settings = mp_graph->GetData<PostProcessingSettings>("PPS")->bloom;
	auto* p_output = mp_graph->GetData<Texture2D>("OutCol");
	auto* p_input = mp_graph->GetData<Texture2D>("BloomInCol");

	const auto& spec = p_input->GetSpec();
	unsigned width = spec.width;
	unsigned height = spec.height;

	GL_StateManager::BindTexture(GL_TEXTURE_2D, p_input->GetTextureHandle(), GL_TEXTURE1);
	m_bloom_threshold_shader.ActivateProgram();
	m_bloom_threshold_shader.SetUniform("u_threshold", bloom_settings.threshold);
	m_bloom_threshold_shader.SetUniform("u_knee", bloom_settings.knee);
	// Isolate bright spots
	glBindImageTexture(0, m_bloom_tex.GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glDispatchCompute((GLuint)glm::ceil((float)width / 16.f), (GLuint)glm::ceil((float)height / 16.f), 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	// Downsample passes
	GL_StateManager::BindTexture(GL_TEXTURE_2D, m_bloom_tex.GetTextureHandle(), GL_StateManager::TextureUnits::BLOOM, true);
	const int max_mip_layer = 6;
	m_bloom_downsample_shader.ActivateProgram();
	for (int i = 1; i < max_mip_layer + 1; i++) {
		m_bloom_downsample_shader.SetUniform("u_mip_level", i);
		glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, m_bloom_tex.GetTextureHandle(), i, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
		glDispatchCompute((GLuint)glm::ceil(((float)width / 32.f) / (float)i), (GLuint)glm::ceil(((float)height / 32.f) / (float)i), 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	// Upsample passes
	m_bloom_upsample_shader.ActivateProgram();
	for (int i = max_mip_layer - 1; i >= 0; i--) {
		m_bloom_upsample_shader.SetUniform("u_mip_level", i);
		glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, m_bloom_tex.GetTextureHandle(), i, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
		glDispatchCompute((GLuint)glm::ceil(((float)width / 16.f) / (float)(i + 1)), (GLuint)glm::ceil(((float)height / 16.f) / (float)(i + 1)), 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	m_composition_shader.ActivateProgram();
	m_composition_shader.SetUniform("u_bloom_intensity", bloom_settings.intensity);
	GL_StateManager::BindTexture(GL_TEXTURE_2D, p_output->GetTextureHandle(), GL_TEXTURE0, true);
	glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, p_output->GetTextureHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
	glDispatchCompute((GLuint)glm::ceil((float)width / 8.f), (GLuint)glm::ceil((float)height / 8.f), 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
