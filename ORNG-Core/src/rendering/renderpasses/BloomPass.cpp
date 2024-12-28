#include "pch/pch.h"
#include "core/GLStateManager.h"
#include "core/Window.h"
#include "rendering/renderpasses/BloomPass.h"
#include "rendering/Renderer.h"
#include "shaders/Shader.h"

using namespace ORNG;

void BloomPass::Init() {
	ResizeTexture(ceil(Window::GetWidth() * 0.5f), ceil(Window::GetHeight() * 0.5f));

	m_window_listener.OnEvent = [this](const Events::WindowEvent& _event) {
		if (_event.event_type == Events::WindowEvent::WINDOW_RESIZE) {
			ResizeTexture(ceil(_event.new_window_size.x * 0.5f), ceil(_event.new_window_size.y * 0.5f));
		}
	};
	Events::EventManager::RegisterListener(m_window_listener);

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

void BloomPass::ResizeTexture(unsigned new_width, unsigned new_height) {
	Texture2DSpec bloom_rgb_spec;
	bloom_rgb_spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
	bloom_rgb_spec.mag_filter = GL_LINEAR;
	bloom_rgb_spec.generate_mipmaps = true;
	bloom_rgb_spec.width = new_width;
	bloom_rgb_spec.height = new_height;
	bloom_rgb_spec.format = GL_RGBA;
	bloom_rgb_spec.internal_format = GL_RGBA16F;
	bloom_rgb_spec.wrap_params = GL_CLAMP_TO_EDGE;
	bloom_rgb_spec.storage_type = GL_FLOAT;

	m_bloom_tex.SetSpec(bloom_rgb_spec);
}

void BloomPass::DoPass(Texture2D* p_input, Texture2D* p_output, float intensity, float threshold, float knee) {
	const auto& spec = p_input->GetSpec();
	unsigned width = spec.width;
	unsigned height = spec.height;

	GL_StateManager::BindTexture(GL_TEXTURE_2D, p_input->GetTextureHandle(), GL_TEXTURE1);
	mp_bloom_threshold_shader->ActivateProgram();
	mp_bloom_threshold_shader->SetUniform("u_threshold", threshold);
	mp_bloom_threshold_shader->SetUniform("u_knee", knee);
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
	mp_composition_shader->SetUniform("u_bloom_intensity", intensity);
	glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, p_output->GetTextureHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
	glDispatchCompute((GLuint)glm::ceil((float)width / 8.f), (GLuint)glm::ceil((float)height / 8.f), 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

}
