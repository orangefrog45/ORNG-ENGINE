#include "pch/pch.h"
#include "core/GLStateManager.h"
#include "components/systems/CameraSystem.h"
#include "rendering/renderpasses/PostProcessPass.h"
#include "rendering/renderpasses/FogPass.h"
#include "rendering/RenderGraph.h"
#include "rendering/Renderer.h"
#include "util/Timers.h"
#include "scene/Scene.h"

using namespace ORNG;

void PostProcessPass::Init() {
	mp_scene = mp_graph->GetData<Scene>("Scene");

	m_post_process_shader.AddStage(GL_COMPUTE_SHADER, "res/core-res/shaders/PostProcessCS.glsl");
	m_post_process_shader.Init();
	m_post_process_shader.AddUniforms("exposure", "u_bloom_intensity", "u_fog_enabled");

	m_post_process_shader.AddUniforms({
		"quad_sampler",
		"world_position_sampler",
		"camera_pos",
		});

	if (auto* p_fog_pass = mp_graph->GetRenderpass<FogPass>())
		mp_fog_tex = &p_fog_pass->GetFinalFogTex();
};

void PostProcessPass::DoPass() {
	ORNG_PROFILE_FUNC_GPU();
	auto* p_output_tex = mp_graph->GetData<Texture2D>("OutCol");
	auto* p_cam = mp_scene->GetSystem<CameraSystem>().GetActiveCamera();

	GL_StateManager::BindTexture(GL_TEXTURE_2D, p_output_tex->GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR);
	auto& spec = p_output_tex->GetSpec();
	//DoBloomPass(p_output_tex, spec.width, spec.height, mp_scene);

	m_post_process_shader.ActivateProgram();
	m_post_process_shader.SetUniform("exposure", p_cam ? p_cam->exposure : 1.f);
	m_post_process_shader.SetUniform("u_bloom_intensity", mp_scene->post_processing.bloom.intensity);
	m_post_process_shader.SetUniform("u_fog_enabled", mp_fog_tex && mp_scene->post_processing.global_fog.density_coef >= 0.001f && mp_scene->post_processing.global_fog.step_count != 0);
	
	if (mp_fog_tex)
		GL_StateManager::BindTexture(GL_TEXTURE_2D, mp_fog_tex->GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR_2, false);

	glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, p_output_tex->GetTextureHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
	glDispatchCompute((GLuint)glm::ceil((float)spec.width / 8.f), (GLuint)glm::ceil((float)spec.height / 8.f), 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
};

void PostProcessPass::Destroy() {

};
