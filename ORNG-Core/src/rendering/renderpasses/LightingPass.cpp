#include "pch/pch.h"
#include "components/systems/PointlightSystem.h"
#include "components/systems/SpotlightSystem.h"
#include "core/GLStateManager.h"
#include "rendering/renderpasses/LightingPass.h"

#include <components/systems/EnvMapSystem.h>

#include "rendering/renderpasses/GBufferPass.h"
#include "rendering/renderpasses/DepthPass.h"
#include "rendering/renderpasses/VoxelPass.h"
#include "rendering/renderpasses/SSAOPass.h"
#include "rendering/Renderer.h"
#include "rendering/RenderGraph.h"
#include "scene/Scene.h"
#include "util/Timers.h"

using namespace ORNG;

void LightingPass::Init() {
	p_scene = mp_graph->GetData<Scene>("Scene");
	shader.AddStage(GL_COMPUTE_SHADER, "res/core-res/shaders/LightingCS.glsl");
	shader.Init();
	shader.AddUniforms("u_ibl_active", "u_ssao_active");

	if (auto* p_voxel_pass = mp_graph->GetRenderpass<VoxelPass>()) {
		p_voxel_pass = p_voxel_pass;

		cone_trace_shader.AddStage(GL_COMPUTE_SHADER, "res/core-res/shaders/ConeTraceCS.glsl");
		cone_trace_shader.Init();

		auto& output_spec = mp_graph->GetData<Texture2D>("OutCol")->GetSpec();

		Texture2DSpec cone_trace_spec;
		cone_trace_spec.format = GL_RGBA;
		cone_trace_spec.internal_format = GL_RGBA16F;
		cone_trace_spec.storage_type = GL_FLOAT;
		cone_trace_spec.width = static_cast<uint32_t>(output_spec.width * 0.5);
		cone_trace_spec.height = static_cast<uint32_t>(output_spec.height * 0.5);
		cone_trace_accum_tex.SetSpec(cone_trace_spec);

		depth_aware_upsample_sv.SetPath(GL_COMPUTE_SHADER, "res/core-res/shaders/DepthAwareUpsampleCS.glsl");
		// Cone trace upsample pass is also normal-aware as well as depth-aware
		depth_aware_upsample_sv.AddVariant(0, { "CONE_TRACE_UPSAMPLE" }, {});
	}

	auto* p_gbuffer_pass = mp_graph->GetRenderpass<GBufferPass>();
	ASSERT(p_gbuffer_pass);
	p_gbf_albedo_tex = &p_gbuffer_pass->albedo;
	p_gbf_normal_tex = &p_gbuffer_pass->normals;
	p_gbf_rma_tex = &p_gbuffer_pass->rma;
	p_gbf_shader_id_tex = &p_gbuffer_pass->shader_ids;
	p_gbf_depth_tex = &p_gbuffer_pass->depth;

	p_depth_pass = mp_graph->GetRenderpass<DepthPass>();

	if (p_scene->HasSystem<SpotlightSystem>())
		p_spotlight_depth_tex = &p_scene->GetSystem<SpotlightSystem>().GetDepthTex();
	
	if (p_scene->HasSystem<PointlightSystem>())
		p_pointlight_depth_tex = &p_scene->GetSystem<PointlightSystem>().GetDepthTex();

};

void LightingPass::DoPass() {
	auto* p_output_tex = mp_graph->GetData<Texture2D>("OutCol");

	ORNG_PROFILE_FUNC_GPU();

	GL_StateManager::BindTexture(GL_TEXTURE_2D_ARRAY, p_depth_pass->directional_light_depth_tex.GetTextureHandle(), GL_StateManager::TextureUnits::DIR_SHADOW_MAP, true);
	GL_StateManager::BindTexture(GL_TEXTURE_2D_ARRAY, p_spotlight_depth_tex->GetTextureHandle(), GL_StateManager::TextureUnits::SPOT_SHADOW_MAP, false);
	GL_StateManager::BindTexture(GL_TEXTURE_2D, p_gbf_albedo_tex->GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR, false);
	GL_StateManager::BindTexture(GL_TEXTURE_2D, p_gbf_normal_tex->GetTextureHandle(), GL_StateManager::TextureUnits::NORMAL_MAP, false);
	GL_StateManager::BindTexture(GL_TEXTURE_2D, p_gbf_depth_tex->GetTextureHandle(), GL_StateManager::TextureUnits::DEPTH, false);
	GL_StateManager::BindTexture(GL_TEXTURE_2D, p_gbf_shader_id_tex->GetTextureHandle(), GL_StateManager::TextureUnits::SHADER_IDS, false);
	GL_StateManager::BindTexture(GL_TEXTURE_2D, p_gbf_rma_tex->GetTextureHandle(), GL_StateManager::TextureUnits::ROUGHNESS_METALLIC_AO, false);

	if (auto* p_ssao_pass = mp_graph->GetRenderpass<SSAOPass>()) {
		GL_StateManager::BindTexture(GL_TEXTURE_2D, p_ssao_pass->GetSSAOTex().GetTextureHandle(), GL_TEXTURE27, false);
		shader.SetUniform("u_using_ssao", true);
	} else {
		shader.SetUniform("u_using_ssao", false);
	}

	GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, p_pointlight_depth_tex->GetTextureHandle(), GL_StateManager::TextureUnits::POINTLIGHT_DEPTH, false);
	//GL_StateManager::BindTexture(GL_TEXTURE_2D, blue_noise_tex.GetTextureHandle(), GL_StateManager::TextureUnits::BLUE_NOISE, false);

	shader.ActivateProgram();

	if (p_scene->HasSystem<EnvMapSystem>()) {
		auto& skybox = p_scene->GetSystem<EnvMapSystem>().skybox;
		if (skybox.using_env_map) {
			GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, skybox.GetIrradianceTexture()->GetTextureHandle(), GL_StateManager::TextureUnits::DIFFUSE_PREFILTER, false);
			GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, skybox.GetSpecularPrefilter()->GetTextureHandle(), GL_StateManager::TextureUnits::SPECULAR_PREFILTER, false);
		}
		shader.SetUniform("u_ibl_active", skybox.using_env_map);
	} else {
		shader.SetUniform("u_ibl_active", false);
	}


	auto& spec = p_output_tex->GetSpec();
	glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, p_output_tex->GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	GL_StateManager::DispatchCompute((GLuint)glm::ceil((float)spec.width / 8.f), (GLuint)glm::ceil((float)spec.height / 8.f), 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

	if (!p_voxel_pass) return;

	GL_StateManager::BindTexture(GL_TEXTURE_3D, p_voxel_pass->m_scene_voxel_tex_c0.GetTextureHandle(), GL_StateManager::TextureUnits::SCENE_VOXELIZATION, false);
	GL_StateManager::BindTexture(GL_TEXTURE_3D, p_voxel_pass->m_voxel_mip_faces_c0.GetTextureHandle(), GL_TEXTURE5, false);
	GL_StateManager::BindTexture(GL_TEXTURE_3D, p_voxel_pass->m_voxel_mip_faces_c1.GetTextureHandle(), GL_TEXTURE6, false);
	GL_StateManager::BindTexture(GL_TEXTURE_3D, p_voxel_pass->m_scene_voxel_tex_c0.GetTextureHandle(), GL_TEXTURE8, false);
	GL_StateManager::BindTexture(GL_TEXTURE_3D, p_voxel_pass->m_scene_voxel_tex_c1.GetTextureHandle(), GL_TEXTURE9, false);

	// Cone trace at half res
	glClearTexImage(cone_trace_accum_tex.GetTextureHandle(), 0, GL_RGBA, GL_FLOAT, nullptr);
	cone_trace_shader.ActivateProgram();
	glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, cone_trace_accum_tex.GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	GL_StateManager::DispatchCompute((GLuint)glm::ceil((float)spec.width / 16.f), (GLuint)glm::ceil((float)spec.height / 16.f), 1);
	glBindImageTexture(7, cone_trace_accum_tex.GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

	depth_aware_upsample_sv.Activate(0);
	glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, p_output_tex->GetTextureHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
	GL_StateManager::BindTexture(GL_TEXTURE_2D, cone_trace_accum_tex.GetTextureHandle(), GL_TEXTURE23, false);
	GL_StateManager::DispatchCompute((GLuint)glm::ceil((float)spec.width / 8.f), (GLuint)glm::ceil((float)spec.height / 8.f), 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
};

void LightingPass::Destroy() {
};