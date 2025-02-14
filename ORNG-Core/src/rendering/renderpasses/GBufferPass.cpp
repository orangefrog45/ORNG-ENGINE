#include "pch/pch.h"
#include "assets/AssetManager.h"
#include "components/ComponentSystems.h"
#include "core/Window.h"
#include "core/GLStateManager.h"
#include "rendering/renderpasses/GBufferPass.h"
#include "rendering/Renderer.h"
#include "rendering/RenderGraph.h"
#include "rendering/SceneRenderer.h"
#include "shaders/Shader.h"
#include "util/Timers.h"
using namespace ORNG;

enum class GBufferVariants {
	TERRAIN,
	MESH,
	PARTICLE,
	SKYBOX,
	BILLBOARD,
	PARTICLE_BILLBOARD,
	UNIFORM_TRANSFORM
};

void GBufferPass::Init() {
	mp_scene = mp_graph->GetData<Scene>("Scene");
	mp_framebuffer = &Renderer::GetFramebufferLibrary().CreateFramebuffer("gbuffer", true);

	Texture2DSpec gbuffer_spec_2;
	gbuffer_spec_2.format = GL_RED_INTEGER;
	gbuffer_spec_2.internal_format = GL_R8UI;
	gbuffer_spec_2.storage_type = GL_UNSIGNED_INT;
	gbuffer_spec_2.width = Window::GetWidth();
	gbuffer_spec_2.height = Window::GetHeight();

	Texture2DSpec low_pres_spec;
	low_pres_spec.format = GL_RGBA;
	low_pres_spec.internal_format = GL_RGBA16F;
	low_pres_spec.storage_type = GL_FLOAT;
	low_pres_spec.width = Window::GetWidth();
	low_pres_spec.height = Window::GetHeight();

	Texture2DSpec gbuffer_depth_spec;
	gbuffer_depth_spec.format = GL_DEPTH_COMPONENT;
	gbuffer_depth_spec.internal_format = GL_DEPTH_COMPONENT24;
	gbuffer_depth_spec.storage_type = GL_FLOAT;
	gbuffer_depth_spec.min_filter = GL_NEAREST;
	gbuffer_depth_spec.mag_filter = GL_NEAREST;
	gbuffer_depth_spec.width = Window::GetWidth();
	gbuffer_depth_spec.height = Window::GetHeight();

	normals.SetSpec(low_pres_spec);
	albedo.SetSpec(low_pres_spec);
	rma.SetSpec(low_pres_spec);
	shader_ids.SetSpec(gbuffer_spec_2);
	depth.SetSpec(gbuffer_depth_spec);
	normals.OnResize = [this] {
		mp_framebuffer->BindTexture2D(normals.GetTextureHandle(), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
		mp_framebuffer->BindTexture2D(albedo.GetTextureHandle(), GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D);
		mp_framebuffer->BindTexture2D(rma.GetTextureHandle(), GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D);
		mp_framebuffer->BindTexture2D(shader_ids.GetTextureHandle(), GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D);
		mp_framebuffer->BindTexture2D(depth.GetTextureHandle(), GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D);
		};
	normals.OnResize();
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	mp_framebuffer->EnableDrawBuffers(4, buffers);

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
	ptcl_uniforms.push_back("u_transform_start_index");

	mp_displacement_sv = &Renderer::GetShaderLibrary().CreateShaderVariants("SR gbuffer displaced");
	mp_displacement_sv->SetPath(GL_VERTEX_SHADER, "res/core-res/shaders/GBufferVS.glsl");
	mp_displacement_sv->SetPath(GL_FRAGMENT_SHADER, "res/core-res/shaders/GBufferFS.glsl");
	mp_displacement_sv->SetPath(GL_TESS_CONTROL_SHADER, "res/core-res/shaders/GBufferTCS.glsl");
	mp_displacement_sv->SetPath(GL_TESS_EVALUATION_SHADER, "res/core-res/shaders/GBufferTES.glsl");
	mp_displacement_sv->AddVariant(0, { "TESSELLATE" }, gbuffer_uniforms);

	mp_sv = &Renderer::GetShaderLibrary().CreateShaderVariants("gbuffer");
	mp_sv->SetPath(GL_VERTEX_SHADER, "res/core-res/shaders/GBufferVS.glsl");
	mp_sv->SetPath(GL_FRAGMENT_SHADER, "res/core-res/shaders/GBufferFS.glsl");
	{
		using enum GBufferVariants;
		mp_sv->AddVariant((unsigned)TERRAIN, { "TERRAIN_MODE" }, gbuffer_uniforms);
		mp_sv->AddVariant((unsigned)MESH, {}, gbuffer_uniforms);
		mp_sv->AddVariant((unsigned)PARTICLE, { "PARTICLE" }, ptcl_uniforms);
		mp_sv->AddVariant((unsigned)SKYBOX, { "SKYBOX_MODE" }, {});
		mp_sv->AddVariant((unsigned)BILLBOARD, { "BILLBOARD" }, gbuffer_uniforms);
		mp_sv->AddVariant((unsigned)PARTICLE_BILLBOARD, { "PARTICLE", "BILLBOARD" }, ptcl_uniforms);

		std::vector<std::string> transform_uniforms = gbuffer_uniforms;
		transform_uniforms.push_back("u_transform");
		mp_sv->AddVariant((unsigned)UNIFORM_TRANSFORM, { "UNIFORM_TRANSFORM" }, transform_uniforms);
	}
}

void GBufferPass::DoPass() {
	ORNG_PROFILE_FUNC_GPU();
	auto& out_spec = mp_graph->GetData<Texture2D>("OutCol")->GetSpec();
	glViewport(0, 0, out_spec.width, out_spec.height);

	mp_framebuffer->Bind();

	GL_StateManager::DefaultClearBits();
	GL_StateManager::ClearBitsUnsignedInt();

	using enum GBufferVariants;

	auto& mesh_sys = mp_scene->GetSystem<MeshInstancingSystem>();

	// Draw tessellated meshes
	mp_displacement_sv->Activate(0);
	mp_displacement_sv->SetUniform("u_bloom_threshold", mp_scene->post_processing.bloom.threshold);
	glPatchParameteri(GL_PATCH_VERTICES, 3);
	for (const auto* group : mesh_sys.GetInstanceGroups()) {
		SceneRenderer::DrawInstanceGroupGBuffer(mp_displacement_sv, group, SOLID, ORNG_MatFlags_TESSELLATED, ORNG_MatFlags_INVALID, GL_PATCHES);
	}

	mp_sv->Activate((unsigned)GBufferVariants::MESH);
	mp_sv->SetUniform("u_bloom_threshold", mp_scene->post_processing.bloom.threshold);
	//Draw all meshes in scene (instanced)
	for (const auto* group : mesh_sys.GetInstanceGroups()) {
		SceneRenderer::DrawInstanceGroupGBuffer(mp_sv, group, SOLID, ORNG_DEFAULT_VERT_FRAG_MAT_FLAGS, ORNG_MatFlags_TESSELLATED, true);
	}


	mp_sv->Activate((unsigned)GBufferVariants::BILLBOARD);
	for (const auto* group : mesh_sys.GetBillboardInstanceGroups()) {
		SceneRenderer::DrawInstanceGroupGBuffer(mp_sv, group, SOLID, ORNG_DEFAULT_VERT_FRAG_MAT_FLAGS, ORNG_MatFlags_TESSELLATED, true);
	}

	//RenderVehicles(mp_gbuffer_shader_mesh_bufferless, RenderGroup::SOLID);
	if (mp_scene->HasSystem<ParticleSystem>()) {
		GL_StateManager::BindSSBO(mp_scene->GetSystem<ParticleSystem>().m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);
		mp_sv->Activate((unsigned)GBufferVariants::PARTICLE);
		for (auto [entity, emitter, res] : mp_scene->GetRegistry().view<ParticleEmitterComponent, ParticleMeshResources>().each()) {
			if (!emitter.AreAnyEmittedParticlesAlive()) continue;
			mp_sv->SetUniform("u_transform_start_index", emitter.GetParticleStartIdx());
			SceneRenderer::DrawMeshGBuffer(mp_sv, res.p_mesh, SOLID, emitter.GetNbParticles(), &res.materials[0],
				ORNG_DEFAULT_VERT_FRAG_MAT_FLAGS, ORNG_MatFlags_TESSELLATED, true);
		}
	}

	mp_sv->Activate((unsigned)GBufferVariants::PARTICLE_BILLBOARD);
	auto* p_quad_mesh = AssetManager::GetAsset<MeshAsset>(ORNG_BASE_QUAD_ID);
	for (auto [entity, emitter, res] : mp_scene->GetRegistry().view<ParticleEmitterComponent, ParticleBillboardResources>().each()) {
		if (!emitter.AreAnyEmittedParticlesAlive()) continue;
		mp_sv->SetUniform("u_transform_start_index", emitter.GetParticleStartIdx());
		SceneRenderer::DrawMeshGBuffer(mp_sv, p_quad_mesh, SOLID, emitter.GetNbParticles(), &res.p_material,
			ORNG_DEFAULT_VERT_FRAG_MAT_FLAGS, ORNG_MatFlags_TESSELLATED, true);
	}

	mp_sv->Activate((unsigned)GBufferVariants::UNIFORM_TRANSFORM);
	//RenderVehicles(mp_sv, SOLID, mp_scene);

	//mp_sv->Activate((unsigned)GBufferVariants::TERRAIN);
	//SetGBufferMaterial(mp_sv, p_scene->terrain.mp_material);
	//mp_sv->SetUniform<unsigned int>("u_shader_id", ShaderLibrary::LIGHTING_SHADER_ID);
	//DrawTerrain(p_cam);

	// Draw skybox
	mp_sv->Activate((unsigned)GBufferVariants::SKYBOX);
	GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, mp_scene->skybox.GetSkyboxTexture().GetTextureHandle(),
		GL_StateManager::TextureUnits::COLOUR_CUBEMAP, false);
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);
	Renderer::DrawCube();
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
}

void GBufferPass::Destroy() {
	normals.Unload();
	albedo.Unload();
	depth.Unload();
	rma.Unload();
	shader_ids.Unload();

	Renderer::GetFramebufferLibrary().DeleteFramebuffer(mp_framebuffer);
	auto& shader_lib = Renderer::GetShaderLibrary();
	shader_lib.DeleteShader(mp_sv->GetName());
	shader_lib.DeleteShader(mp_displacement_sv->GetName());
}