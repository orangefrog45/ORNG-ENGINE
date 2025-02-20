#include "pch/pch.h"
#include "core/FrameTiming.h"
#include "core/GLStateManager.h"
#include "rendering/renderpasses/FogPass.h"
#include "rendering/renderpasses/GBufferPass.h"
#include "rendering/Renderer.h"
#include "rendering/RenderGraph.h"
#include "scene/Scene.h"
#include "shaders/Shader.h"
#include "util/Timers.h"

using namespace ORNG;


void FogPass::Init() {
	mp_depth_tex = &mp_graph->GetRenderpass<GBufferPass>()->depth;
	mp_scene = mp_graph->GetData<Scene>("Scene");
	m_blur_shader.AddStage(GL_COMPUTE_SHADER, "res/core-res/shaders/BlurFS.glsl");
	m_blur_shader.Init();
	m_blur_shader.AddUniform("u_horizontal");

	m_depth_aware_upsample_sv.SetPath(GL_COMPUTE_SHADER, "res/core-res/shaders/DepthAwareUpsampleCS.glsl");
	m_depth_aware_upsample_sv.AddVariant(0, {}, {});

	auto& out_spec = mp_graph->GetData<Texture2D>("OutCol")->GetSpec();

	Texture2DSpec rgba16_spec; // Could probably get by with rgba8
	rgba16_spec.format = GL_RGBA;
	rgba16_spec.internal_format = GL_RGBA16F;
	rgba16_spec.storage_type = GL_FLOAT;
	rgba16_spec.width = out_spec.width;
	rgba16_spec.height = out_spec.height;
	rgba16_spec.min_filter = GL_NEAREST;
	rgba16_spec.mag_filter = GL_NEAREST;
	rgba16_spec.wrap_params = GL_CLAMP_TO_EDGE;

	m_fog_blur_tex_1.SetSpec(rgba16_spec);
	m_fog_blur_tex_2.SetSpec(rgba16_spec);

	m_fog_shader.AddStage(GL_COMPUTE_SHADER, "res/core-res/shaders/FogCS.glsl");
	m_fog_shader.Init();
	m_fog_shader.AddUniforms({
		"u_fog_colour",
		"u_time",
		"u_scattering_anisotropy",
		"u_absorption_coef",
		"u_scattering_coef",
		"u_density_coef",
		"u_step_count",
		"u_emissive",
		"u_dir_light_matrices[0]",
		"u_dir_light_matrices[1]",
		"u_dir_light_matrices[2]",
		});

	// Fog texture
	Texture2DSpec fog_overlay_spec = rgba16_spec;
	fog_overlay_spec.width = glm::ceil(out_spec.width / 2.f);
	fog_overlay_spec.height = glm::ceil(out_spec.height / 2.f);
	m_fog_output_tex.SetSpec(fog_overlay_spec);
};

void FogPass::DoPass() {
	ORNG_PROFILE_FUNC_GPU();

	if (mp_scene->post_processing.global_fog.density_coef < 0.001f || mp_scene->post_processing.global_fog.step_count == 0) {
		return;
	}

	//draw fog texture
	m_fog_shader.ActivateProgram();

	m_fog_shader.SetUniform("u_scattering_coef", mp_scene->post_processing.global_fog.scattering_coef);
	m_fog_shader.SetUniform("u_absorption_coef", mp_scene->post_processing.global_fog.absorption_coef);
	m_fog_shader.SetUniform("u_density_coef", mp_scene->post_processing.global_fog.density_coef);
	m_fog_shader.SetUniform("u_scattering_anisotropy", mp_scene->post_processing.global_fog.scattering_anisotropy);
	m_fog_shader.SetUniform("u_fog_colour", mp_scene->post_processing.global_fog.colour);
	m_fog_shader.SetUniform("u_step_count", mp_scene->post_processing.global_fog.step_count);
	m_fog_shader.SetUniform("u_time", static_cast<float>(FrameTiming::GetTotalElapsedTime()));
	m_fog_shader.SetUniform("u_emissive", mp_scene->post_processing.global_fog.emissive_factor);

	GL_StateManager::BindTexture(GL_TEXTURE_2D, mp_depth_tex->GetTextureHandle(), GL_StateManager::TextureUnits::DEPTH, false);
	auto& out_spec = mp_graph->GetData<Texture2D>("OutCol")->GetSpec();

	glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, m_fog_output_tex.GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glDispatchCompute((GLuint)glm::ceil((float)out_spec.width / 16.f), (GLuint)glm::ceil((float)out_spec.height / 16.f), 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//Upsample fog texture
	m_depth_aware_upsample_sv.Activate(0);
	GL_StateManager::BindTexture(GL_TEXTURE_2D, m_fog_output_tex.GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR_3);

	glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, m_fog_blur_tex_1.GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glDispatchCompute((GLuint)glm::ceil((float)out_spec.width / 8.f), (GLuint)glm::ceil((float)out_spec.height / 8.f), 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//blur fog texture
	m_blur_shader.ActivateProgram();

	for (int i = 0; i < 2; i++) {
		m_blur_shader.SetUniform("u_horizontal", 1);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_fog_blur_tex_1.GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR_3);
		glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, m_fog_blur_tex_2.GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		glDispatchCompute((GLuint)glm::ceil((float)out_spec.width / 8.f), (GLuint)glm::ceil((float)out_spec.height / 8.f), 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		m_blur_shader.SetUniform("u_horizontal", 0);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_fog_blur_tex_2.GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR_3);
		glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, m_fog_blur_tex_1.GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		glDispatchCompute((GLuint)glm::ceil((float)out_spec.width / 8.f), (GLuint)glm::ceil((float)out_spec.height / 8.f), 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
};

void FogPass::Destroy() {
};