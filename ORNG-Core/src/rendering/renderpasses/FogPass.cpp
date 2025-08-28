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
	p_depth_tex = &mp_graph->GetRenderpass<GBufferPass>()->depth;
	p_scene = mp_graph->GetData<Scene>("Scene");
	blur_shader.AddStage(GL_COMPUTE_SHADER, "res/core-res/shaders/BlurFS.glsl");
	blur_shader.Init();
	blur_shader.AddUniform("u_horizontal");

	depth_aware_upsample_sv.SetPath(GL_COMPUTE_SHADER, "res/core-res/shaders/DepthAwareUpsampleCS.glsl");
	depth_aware_upsample_sv.AddVariant(0, {}, {});

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

	fog_blur_tex_1.SetSpec(rgba16_spec);
	fog_blur_tex_2.SetSpec(rgba16_spec);

	fog_shader.AddStage(GL_COMPUTE_SHADER, "res/core-res/shaders/FogCS.glsl");
	fog_shader.Init();
	fog_shader.AddUniforms({
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
	fog_overlay_spec.width = static_cast<uint32_t>(glm::ceil(static_cast<float>(out_spec.width) / 2.f));
	fog_overlay_spec.height = static_cast<uint32_t>(glm::ceil(static_cast<float>(out_spec.height) / 2.f));
	fog_output_tex.SetSpec(fog_overlay_spec);
};

void FogPass::DoPass() {
	ORNG_PROFILE_FUNC_GPU();

	if (p_scene->post_processing.global_fog.density_coef < 0.001f || p_scene->post_processing.global_fog.step_count == 0) {
		return;
	}

	//draw fog texture
	fog_shader.ActivateProgram();

	fog_shader.SetUniform("u_scattering_coef", p_scene->post_processing.global_fog.scattering_coef);
	fog_shader.SetUniform("u_absorption_coef", p_scene->post_processing.global_fog.absorption_coef);
	fog_shader.SetUniform("u_density_coef", p_scene->post_processing.global_fog.density_coef);
	fog_shader.SetUniform("u_scattering_anisotropy", p_scene->post_processing.global_fog.scattering_anisotropy);
	fog_shader.SetUniform("u_fog_colour", p_scene->post_processing.global_fog.colour);
	fog_shader.SetUniform("u_step_count", p_scene->post_processing.global_fog.step_count);
	fog_shader.SetUniform("u_time", static_cast<float>(FrameTiming::GetTotalElapsedTime()));
	fog_shader.SetUniform("u_emissive", p_scene->post_processing.global_fog.emissive_factor);

	GL_StateManager::BindTexture(GL_TEXTURE_2D, p_depth_tex->GetTextureHandle(), GL_StateManager::TextureUnits::DEPTH, false);
	auto& out_spec = mp_graph->GetData<Texture2D>("OutCol")->GetSpec();

	glBindImageTexture(
		GL_StateManager::TextureUnitIndexes::COLOUR,
		fog_output_tex.GetTextureHandle(),
		0,
		GL_FALSE,
		0,
		GL_WRITE_ONLY,
		GL_RGBA16F
	);

	glDispatchCompute(
		static_cast<GLuint>(glm::ceil(static_cast<float>(out_spec.width) / 16.f)),
		static_cast<GLuint>(glm::ceil(static_cast<float>(out_spec.height) / 16.f)),
		1
	);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	// Upsample fog texture
	depth_aware_upsample_sv.Activate(0);
	GL_StateManager::BindTexture(GL_TEXTURE_2D, fog_output_tex.GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR_3);

	glBindImageTexture(
		GL_StateManager::TextureUnitIndexes::COLOUR,
		fog_blur_tex_1.GetTextureHandle(),
		0,
		GL_FALSE,
		0,
		GL_WRITE_ONLY,
		GL_RGBA16F
	);

	glDispatchCompute(
		static_cast<GLuint>(glm::ceil(static_cast<float>(out_spec.width) / 8.f)),
		static_cast<GLuint>(glm::ceil(static_cast<float>(out_spec.height) / 8.f)),
		1
	);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//blur fog texture
	blur_shader.ActivateProgram();

	for (int i = 0; i < 2; i++) {
		blur_shader.SetUniform("u_horizontal", 1);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, fog_blur_tex_1.GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR_3);

		glBindImageTexture(
			GL_StateManager::TextureUnitIndexes::COLOUR,
			fog_blur_tex_2.GetTextureHandle(),
			0,
			GL_FALSE,
			0,
			GL_WRITE_ONLY,
			GL_RGBA16F
		);

		glDispatchCompute(
			static_cast<GLuint>(glm::ceil(static_cast<float>(out_spec.width) / 8.f)),
			static_cast<GLuint>(glm::ceil(static_cast<float>(out_spec.height) / 8.f)),
			1
		);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		blur_shader.SetUniform("u_horizontal", 0);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, fog_blur_tex_2.GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR_3);

		glBindImageTexture(
			GL_StateManager::TextureUnitIndexes::COLOUR,
			fog_blur_tex_1.GetTextureHandle(),
			0,
			GL_FALSE,
			0,
			GL_WRITE_ONLY,
			GL_RGBA16F
		);

		glDispatchCompute(
			static_cast<GLuint>(glm::ceil(static_cast<float>(out_spec.width) / 8.f)),
			static_cast<GLuint>(glm::ceil(static_cast<float>(out_spec.height) / 8.f)),
			1
		);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
};

