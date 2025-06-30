#include "pch/pch.h"
#include "assets/AssetManager.h"
#include "core/Window.h"
#include "core/GLStateManager.h"
#include "rendering/renderpasses/GBufferPass.h"

#include <components/systems/EnvMapSystem.h>

#include "rendering/Renderer.h"
#include "rendering/RenderGraph.h"
#include "rendering/SceneRenderer.h"
#include "shaders/Shader.h"
#include "util/Timers.h"
#include "components/systems/MeshInstancingSystem.h"
#include "components/systems/ParticleSystem.h"
#include "components/DecalComponent.h"

using namespace ORNG;

enum class GBufferVariants {
	TERRAIN,
	MESH,
	PARTICLE,
	SKYBOX,
	BILLBOARD,
	PARTICLE_BILLBOARD,
	UNIFORM_TRANSFORM,
	DECAL,
};

void GBufferPass::Init() {
	const auto& output_spec = mp_graph->GetData<Texture2D>("OutCol")->GetSpec();

	mp_scene = mp_graph->GetData<Scene>("Scene");
	m_framebuffer.Init();

	Texture2DSpec gbuffer_spec_2;
	gbuffer_spec_2.format = GL_RED_INTEGER;
	gbuffer_spec_2.internal_format = GL_R8UI;
	gbuffer_spec_2.storage_type = GL_UNSIGNED_INT;
	gbuffer_spec_2.width = output_spec.width;
	gbuffer_spec_2.height = output_spec.height;

	Texture2DSpec low_pres_spec;
	low_pres_spec.format = GL_RGBA;
	low_pres_spec.internal_format = GL_RGBA16F;
	low_pres_spec.storage_type = GL_FLOAT;
	low_pres_spec.width = output_spec.width;
	low_pres_spec.height = output_spec.height;

	Texture2DSpec gbuffer_depth_spec;
	gbuffer_depth_spec.format = GL_DEPTH_COMPONENT;
	gbuffer_depth_spec.internal_format = GL_DEPTH_COMPONENT24;
	gbuffer_depth_spec.storage_type = GL_FLOAT;
	gbuffer_depth_spec.min_filter = GL_NEAREST;
	gbuffer_depth_spec.mag_filter = GL_NEAREST;
	gbuffer_depth_spec.width = output_spec.width;
	gbuffer_depth_spec.height = output_spec.height;

	normals.SetSpec(low_pres_spec);
	albedo.SetSpec(low_pres_spec);
	rma.SetSpec(low_pres_spec);
	shader_ids.SetSpec(gbuffer_spec_2);
	depth.SetSpec(gbuffer_depth_spec);
	m_framebuffer.BindTexture2D(normals.GetTextureHandle(), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
	m_framebuffer.BindTexture2D(albedo.GetTextureHandle(), GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D);
	m_framebuffer.BindTexture2D(rma.GetTextureHandle(), GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D);
	m_framebuffer.BindTexture2D(shader_ids.GetTextureHandle(), GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D);
	m_framebuffer.BindTexture2D(depth.GetTextureHandle(), GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D);
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	m_framebuffer.EnableDrawBuffers(4, buffers);

	std::vector<std::string> gbuffer_uniforms = SceneRenderer::GetGBufferUniforms();

	std::vector<std::string> ptcl_uniforms = gbuffer_uniforms;
	ptcl_uniforms.push_back("u_transform_start_index");

	m_displacement_sv.SetPath(GL_VERTEX_SHADER, "res/core-res/shaders/GBufferVS.glsl");
	m_displacement_sv.SetPath(GL_FRAGMENT_SHADER, "res/core-res/shaders/GBufferFS.glsl");
	m_displacement_sv.SetPath(GL_TESS_CONTROL_SHADER, "res/core-res/shaders/GBufferTCS.glsl");
	m_displacement_sv.SetPath(GL_TESS_EVALUATION_SHADER, "res/core-res/shaders/GBufferTES.glsl");
	m_displacement_sv.AddVariant(0, { "TESSELLATE" }, gbuffer_uniforms);

	m_sv.SetPath(GL_VERTEX_SHADER, "res/core-res/shaders/GBufferVS.glsl");
	m_sv.SetPath(GL_FRAGMENT_SHADER, "res/core-res/shaders/GBufferFS.glsl");
	{
		using enum GBufferVariants;
		m_sv.AddVariant((unsigned)TERRAIN, { "TERRAIN_MODE" }, gbuffer_uniforms);
		m_sv.AddVariant((unsigned)MESH, {}, gbuffer_uniforms);
		m_sv.AddVariant((unsigned)PARTICLE, { "PARTICLE" }, ptcl_uniforms);
		m_sv.AddVariant((unsigned)SKYBOX, { "SKYBOX_MODE" }, {});
		m_sv.AddVariant((unsigned)BILLBOARD, { "BILLBOARD" }, gbuffer_uniforms);
		m_sv.AddVariant((unsigned)PARTICLE_BILLBOARD, { "PARTICLE", "BILLBOARD" }, ptcl_uniforms);

		std::vector<std::string> transform_uniforms = gbuffer_uniforms;
		transform_uniforms.push_back("u_transform");
		m_sv.AddVariant((unsigned)UNIFORM_TRANSFORM, { "UNIFORM_TRANSFORM" }, transform_uniforms);
		m_sv.AddVariant((unsigned)DECAL, { "UNIFORM_TRANSFORM", "DECAL" }, transform_uniforms);
	}
}

void GBufferPass::DoPass() {
	ORNG_PROFILE_FUNC_GPU();
	auto& out_spec = mp_graph->GetData<Texture2D>("OutCol")->GetSpec();
	glViewport(0, 0, out_spec.width, out_spec.height);

	m_framebuffer.Bind();

	GL_StateManager::DefaultClearBits();
	GL_StateManager::ClearBitsUnsignedInt();

	using enum GBufferVariants;

	auto& mesh_sys = mp_scene->GetSystem<MeshInstancingSystem>();

	// Draw tessellated meshes
	m_displacement_sv.Activate(0);
	m_displacement_sv.SetUniform("u_bloom_threshold", mp_scene->post_processing.bloom.threshold);
	glPatchParameteri(GL_PATCH_VERTICES, 3);
	for (const auto* group : mesh_sys.GetInstanceGroups()) {
		SceneRenderer::DrawInstanceGroupGBuffer(&m_displacement_sv, group, SOLID, ORNG_MatFlags_TESSELLATED, ORNG_MatFlags_INVALID, GL_PATCHES);
	}

	m_sv.Activate((unsigned)GBufferVariants::MESH);
	m_sv.SetUniform("u_bloom_threshold", mp_scene->post_processing.bloom.threshold);
	//Draw all meshes in scene (instanced)
	for (const auto* group : mesh_sys.GetInstanceGroups()) {
		SceneRenderer::DrawInstanceGroupGBuffer(&m_sv, group, SOLID, ORNG_DEFAULT_VERT_FRAG_MAT_FLAGS, ORNG_MatFlags_TESSELLATED, true);
	}


	m_sv.Activate((unsigned)GBufferVariants::BILLBOARD);
	for (const auto* group : mesh_sys.GetBillboardInstanceGroups()) {
		SceneRenderer::DrawInstanceGroupGBuffer(&m_sv, group, SOLID, ORNG_DEFAULT_VERT_FRAG_MAT_FLAGS, ORNG_MatFlags_TESSELLATED, true);
	}

	//RenderVehicles(mp_gbuffer_shader_mesh_bufferless, RenderGroup::SOLID);
	if (mp_scene->HasSystem<ParticleSystem>()) {
		GL_StateManager::BindSSBO(mp_scene->GetSystem<ParticleSystem>().m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);
		m_sv.Activate((unsigned)GBufferVariants::PARTICLE);
		for (auto [entity, emitter, res] : mp_scene->GetRegistry().view<ParticleEmitterComponent, ParticleMeshResources>().each()) {
			if (!emitter.AreAnyEmittedParticlesAlive()) continue;
			m_sv.SetUniform("u_transform_start_index", emitter.GetParticleStartIdx());
			SceneRenderer::DrawMeshGBuffer(&m_sv, res.p_mesh, SOLID, emitter.GetNbParticles(), &res.materials[0],
				ORNG_DEFAULT_VERT_FRAG_MAT_FLAGS, ORNG_MatFlags_TESSELLATED, true);
		}

		m_sv.Activate((unsigned)GBufferVariants::PARTICLE_BILLBOARD);
		auto* p_quad_mesh = AssetManager::GetAsset<MeshAsset>(ORNG_BASE_QUAD_ID);
		for (auto [entity, emitter, res] : mp_scene->GetRegistry().view<ParticleEmitterComponent, ParticleBillboardResources>().each()) {
			if (!emitter.AreAnyEmittedParticlesAlive()) continue;
			m_sv.SetUniform("u_transform_start_index", emitter.GetParticleStartIdx());
			SceneRenderer::DrawMeshGBuffer(&m_sv, p_quad_mesh, SOLID, emitter.GetNbParticles(), &res.p_material,
				ORNG_DEFAULT_VERT_FRAG_MAT_FLAGS, ORNG_MatFlags_TESSELLATED, true);
		}
	}



	m_sv.Activate((unsigned)GBufferVariants::UNIFORM_TRANSFORM);
	//RenderVehicles(mp_sv, SOLID, mp_scene);


	// Draw decals
	glDisable(GL_DEPTH_TEST);
	m_sv.Activate((unsigned)GBufferVariants::DECAL);
	GL_StateManager::BindTexture(GL_TEXTURE_2D, depth.GetTextureHandle(), GL_TEXTURE16);
	for (auto [entity, decal, transform] : mp_scene->GetRegistry().view<DecalComponent, TransformComponent>().each()) {
		// TODO: These should be sorted by material for minimal state changes, DecalSystem could handle this
		if (!decal.p_material) continue;
		SceneRenderer::SetGBufferMaterial(&m_sv, decal.p_material);
		m_sv.SetUniform("u_transform", transform.GetMatrix());
		Renderer::DrawCube();
	}
	glEnable(GL_DEPTH_TEST);

	// Draw skybox
	if (mp_scene->HasSystem<EnvMapSystem>()) {
		auto& skybox = mp_scene->GetSystem<EnvMapSystem>().skybox;
		m_sv.Activate((unsigned)GBufferVariants::SKYBOX);
		GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, skybox.GetSkyboxTexture().GetTextureHandle(),
		GL_StateManager::TextureUnits::COLOUR_CUBEMAP, false);
		glDisable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL);
		Renderer::DrawCube();
		glDepthFunc(GL_LESS);
		glEnable(GL_CULL_FACE);
	}
}

void GBufferPass::Destroy() {
	normals.Unload();
	albedo.Unload();
	depth.Unload();
	rma.Unload();
	shader_ids.Unload();
}