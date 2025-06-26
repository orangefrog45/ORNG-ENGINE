#include "pch/pch.h"
#include "assets/AssetManager.h"
#include "core/GLStateManager.h"
#include "rendering/renderpasses/TransparencyPass.h"
#include "rendering/renderpasses/GBufferPass.h"
#include "rendering/Renderer.h"
#include "rendering/RenderGraph.h"
#include "rendering/SceneRenderer.h"
#include "shaders/Shader.h"
#include "components/systems/MeshInstancingSystem.h"
#include "components/systems/ParticleSystem.h"

using namespace ORNG;

enum class TransparencyShaderVariants {
	DEFAULT,
	T_PARTICLE,
	T_PARTICLE_BILLBOARD,
};


void TransparencyPass::Init() {
	mp_scene = mp_graph->GetData<Scene>("Scene");

	std::vector<std::string> gbuffer_uniforms{
		"u_roughness_sampler_active",
			"u_metallic_sampler_active",
			"u_emissive_sampler_active",
			"u_normal_sampler_active",
			"u_ao_sampler_active",
			"u_displacement_sampler_active",
			"u_num_parallax_layers",
			"u_material.base_colour",
			"u_material.metallic",
			"u_material.roughness",
			"u_material.ao",
			"u_material.tile_scale",
			"u_material.emissive_strength",
			"u_material.flags",
			"u_material.displacement_scale",
			"u_shader_id",
			"u_material.sprite_data.num_rows",
			"u_material.sprite_data.num_cols",
			"u_material.sprite_data.fps"
	};

	std::vector<std::string> ptcl_uniforms = gbuffer_uniforms;
	ptcl_uniforms.emplace_back("u_transform_start_index");

	m_transparency_shader_variants.SetPath(GL_VERTEX_SHADER, "res/core-res/shaders/GBufferVS.glsl");
	m_transparency_shader_variants.SetPath(GL_FRAGMENT_SHADER, "res/core-res/shaders/WeightedBlendedFS.glsl");
	{
		using enum TransparencyShaderVariants;
		m_transparency_shader_variants.AddVariant(static_cast<unsigned>(DEFAULT), { }, gbuffer_uniforms);
		m_transparency_shader_variants.AddVariant(static_cast<unsigned>(T_PARTICLE), { "PARTICLE" }, ptcl_uniforms);
		m_transparency_shader_variants.AddVariant(static_cast<unsigned>(T_PARTICLE_BILLBOARD), { "PARTICLE", "BILLBOARD" }, ptcl_uniforms);
	}

	m_transparency_composite_shader.AddStage(GL_FRAGMENT_SHADER, "res/core-res/shaders/TransparentCompositeFS.glsl");
	m_transparency_composite_shader.AddStage(GL_VERTEX_SHADER, "res/core-res/shaders/QuadVS.glsl");
	m_transparency_composite_shader.Init();

	auto& out_spec = mp_graph->GetData<Texture2D>("OutCol")->GetSpec();
	mp_depth_tex = &mp_graph->GetRenderpass<GBufferPass>()->depth;

	Texture2DSpec low_pres_spec;
	low_pres_spec.format = GL_RGBA;
	low_pres_spec.internal_format = GL_RGBA16F;
	low_pres_spec.storage_type = GL_FLOAT;
	low_pres_spec.width = out_spec.width;
	low_pres_spec.height = out_spec.height;

	Texture2DSpec lp4_spec = low_pres_spec;
	lp4_spec.internal_format = GL_RGBA16F;
	lp4_spec.format = GL_RGBA;
	Texture2DSpec r8_spec = low_pres_spec;
	r8_spec.format = GL_RED;
	r8_spec.internal_format = GL_R8;

	m_transparency_accum.SetSpec(lp4_spec);
	m_transparency_revealage.SetSpec(r8_spec);
	m_transparency_fb.Init();
	m_transparency_fb.BindTexture2D(m_transparency_accum.GetTextureHandle(), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
	m_transparency_fb.BindTexture2D(m_transparency_revealage.GetTextureHandle(), GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D);
	GLenum buffers2[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	m_transparency_fb.EnableDrawBuffers(2, buffers2);

	/* TRANSPARENCY COMPOSITION FB */
	m_composition_fb.Init();
};

void TransparencyPass::DoPass() {
	m_transparency_fb.Bind();
	m_transparency_fb.BindTexture2D(mp_depth_tex->GetTextureHandle(), GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D);
	GL_StateManager::BindTexture(GL_TEXTURE_2D, mp_depth_tex->GetTextureHandle(), GL_StateManager::TextureUnits::VIEW_DEPTH);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glBlendFunci(0, GL_ONE, GL_ONE); // accumulation blend target
	glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR); // revealage blend target
	glBlendEquation(GL_FUNC_ADD);

	static auto filler_0 = glm::vec4(0); static auto filler_1 = glm::vec4(1);
	glClearBufferfv(GL_COLOR, 0, &filler_0[0]);
	glClearBufferfv(GL_COLOR, 1, &filler_1[0]);

	m_transparency_shader_variants.Activate(static_cast<unsigned>(TransparencyShaderVariants::DEFAULT));
	m_transparency_shader_variants.SetUniform("u_bloom_threshold", mp_scene->post_processing.bloom.threshold);

	auto& mesh_system = mp_scene->GetSystem<MeshInstancingSystem>();

	//Draw all meshes in scene (instanced)
	for (const auto* group : mesh_system.GetInstanceGroups()) {
		SceneRenderer::DrawInstanceGroupGBuffer(&m_transparency_shader_variants, group, RenderGroup::ALPHA_TESTED, ORNG_DEFAULT_VERT_FRAG_MAT_FLAGS,
			ORNG_MatFlags_INVALID, true);
	}

	if (mp_scene->HasSystem<ParticleSystem>()) {
		GL_StateManager::BindSSBO(mp_scene->GetSystem<ParticleSystem>().m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);
		m_transparency_shader_variants.Activate(static_cast<unsigned>(TransparencyShaderVariants::T_PARTICLE));
		for (auto [entity, emitter, res] : mp_scene->GetRegistry().view<ParticleEmitterComponent, ParticleMeshResources>().each()) {
			if (!emitter.AreAnyEmittedParticlesAlive()) continue;
			m_transparency_shader_variants.SetUniform("u_transform_start_index", emitter.GetParticleStartIdx());
			SceneRenderer::DrawMeshGBuffer(&m_transparency_shader_variants, res.p_mesh, ALPHA_TESTED, emitter.GetNbParticles(), &res.materials[0],
				ORNG_DEFAULT_VERT_FRAG_MAT_FLAGS, ORNG_MatFlags_INVALID, true);
		}

		m_transparency_shader_variants.Activate(static_cast<unsigned>(TransparencyShaderVariants::T_PARTICLE_BILLBOARD));
		auto* p_quad_mesh = AssetManager::GetAsset<MeshAsset>(ORNG_BASE_QUAD_ID);
		for (auto [entity, emitter, res] : mp_scene->GetRegistry().view<ParticleEmitterComponent, ParticleBillboardResources>().each()) {
			if (!emitter.AreAnyEmittedParticlesAlive()) continue;
			m_transparency_shader_variants.SetUniform("u_transform_start_index", emitter.GetParticleStartIdx());
			SceneRenderer::DrawMeshGBuffer(&m_transparency_shader_variants, p_quad_mesh, ALPHA_TESTED, emitter.GetNbParticles(), &res.p_material,
				ORNG_DEFAULT_VERT_FRAG_MAT_FLAGS, ORNG_MatFlags_INVALID, true);
		}
	}

	//RenderVehicles(mp_transparency_shader_variants, RenderGroup::ALPHA_TESTED);

	m_composition_fb.Bind();
	m_composition_fb.BindTexture2D(mp_graph->GetData<Texture2D>("OutCol")->GetTextureHandle(), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
	m_transparency_composite_shader.ActivateProgram();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_ALWAYS);
	glDisable(GL_DEPTH_TEST);
	GL_StateManager::BindTexture(GL_TEXTURE_2D, mp_depth_tex->GetTextureHandle(), GL_StateManager::TextureUnits::VIEW_DEPTH);
	GL_StateManager::BindTexture(GL_TEXTURE_2D, m_transparency_accum.GetTextureHandle(), GL_TEXTURE0);
	GL_StateManager::BindTexture(GL_TEXTURE_2D, m_transparency_revealage.GetTextureHandle(), GL_TEXTURE1);

	Renderer::DrawQuad();
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
};

void TransparencyPass::Destroy() {
};