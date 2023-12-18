#include "../headers/GameLayer.h"
#include "../../ORNG-Core/headers/core/CodedAssets.h"
#include "imgui.h"
#include "core/Input.h"
#include "EditorLayer.h"
#include "components/ParticleBufferComponent.h"
#include "assets/AssetManager.h"

constexpr unsigned MAP_RESOLUTION = 1024;

namespace ORNG {
	static float map_scale = 1.0;

	enum ParticleUpdateShaderVariant {
		UPDATE,
		INITIALIZE,
		STATE_UPDATE,
	};

	void GameLayer::OnInit() {
		p_scene = std::make_unique<Scene>();
		p_editor_layer->SetScene(&*p_scene);
		// Initialize fractal renderpass
		p_kleinian_shader = &Renderer::GetShaderLibrary().CreateShader("kleinian");
		p_kleinian_shader->AddStage(GL_VERTEX_SHADER,  "res/shaders/QuadVS.glsl");
		p_kleinian_shader->AddStage(GL_FRAGMENT_SHADER,  "res/shaders/FractalFS.glsl");
		p_kleinian_shader->Init();

		p_map_shader = &Renderer::GetShaderLibrary().CreateShader("kleinian-map");
		p_map_shader->AddStage(GL_VERTEX_SHADER,  "res/shaders/QuadVS.glsl");
		p_map_shader->AddStage(GL_FRAGMENT_SHADER,  "res/shaders/FractalMap.glsl");
		p_map_shader->Init();
		p_map_shader->AddUniform("u_scale");

		p_map_fb = &Renderer::GetFramebufferLibrary().CreateFramebuffer("fractal-map", false);
		p_map_tex = std::make_unique<Texture2D>("map");
		Texture2DSpec spec;
		spec.width = MAP_RESOLUTION;
		spec.height = MAP_RESOLUTION;
		spec.storage_type = GL_FLOAT;
		spec.internal_format = GL_RGB16F;
		spec.format = GL_RGB;

		p_map_tex->SetSpec(spec);
		p_map_fb->BindTexture2D(p_map_tex->GetTextureHandle(), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);

		p_particle_update_shader_variants = &Renderer::GetShaderLibrary().CreateShaderVariants("Particle update test layer");
		p_particle_update_shader_variants->SetPath(GL_COMPUTE_SHADER, "res/shaders/ParticleUpdateCS.glsl");
		p_particle_update_shader_variants->AddVariant(ParticleUpdateShaderVariant::UPDATE, {}, {});
		p_particle_update_shader_variants->AddVariant(ParticleUpdateShaderVariant::INITIALIZE, {"INITIALIZE"}, {});
		p_particle_update_shader_variants->AddVariant(ParticleUpdateShaderVariant::STATE_UPDATE, {"STATE_UPDATE"}, {});

		std::vector<std::string> gbuffer_uniforms{
			"u_roughness_sampler_active",
			"u_metallic_sampler_active",
			"u_emissive_sampler_active",
			"u_normal_sampler_active",
			"u_ao_sampler_active",
			"u_displacement_sampler_active",
			"u_num_parallax_layers",
			"u_parallax_height_scale",
			"u_material.base_color",
			"u_material.metallic",
			"u_material.roughness",
			"u_material.ao",
			"u_material.tile_scale",
			"u_material.emissive",
			"u_material.emissive_strength",
			"u_bloom_threshold",
			"u_shader_id",
			"u_material.using_spritesheet",
			"u_material.sprite_data.num_rows",
			"u_material.sprite_data.num_cols",
			"u_material.sprite_data.fps"
		};

		p_particle_render_shader_variants = &Renderer::GetShaderLibrary().CreateShaderVariants("Particle render test layer");
		p_particle_render_shader_variants->SetPath(GL_VERTEX_SHADER,  "res/shaders/GBufferVS.glsl");
		p_particle_render_shader_variants->SetPath(GL_FRAGMENT_SHADER,  "res/shaders/ParticleRenderFS.glsl");
		p_particle_render_shader_variants->AddVariant(0, {"PARTICLE", "PARTICLES_DETACHED"}, gbuffer_uniforms);

		//p_particle_update_shader_variants->Activate(ParticleUpdateShaderVariant::INITIALIZE);
		//GL_StateManager::DispatchCompute(glm::ceil(p_comp->GetCurrentAllocatedParticles() / 32.f), 1, 1);


		Renderpass rp;
		rp.func = [&](RenderResources res) {
			p_kleinian_shader->ActivateProgram();
			Renderer::DrawQuad();
			};

		rp.stage = RenderpassStage::POST_GBUFFER;
		rp.name = "fractal";
		//SceneRenderer::AttachRenderpassIntercept(rp);

		Renderpass particle_rp;
		particle_rp.func = [this](RenderResources res) {
			RenderParticles();
			};
		particle_rp.stage = RenderpassStage::POST_GBUFFER;
		particle_rp.name = "particle render";

		SceneRenderer::AttachRenderpassIntercept(particle_rp);
	}


	void GameLayer::UpdateParticles() {

		auto* p_ent = p_editor_layer->m_active_scene->GetEntity("Pbuf");
			if (!p_ent->GetComponent<ParticleBufferComponent>()) {
			ORNG_CORE_TRACE("NOT FOUND");
			return;
		}

		p_particle_update_shader_variants->Activate(ParticleUpdateShaderVariant::STATE_UPDATE);
		GL_StateManager::BindSSBO(p_ent->GetComponent<ParticleBufferComponent>()->m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);
		GL_StateManager::DispatchCompute(1, 1, 1);

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		p_particle_update_shader_variants->Activate(ParticleUpdateShaderVariant::UPDATE);
		GL_StateManager::BindSSBO(p_ent->GetComponent<ParticleBufferComponent>()->m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);
		GL_StateManager::DispatchCompute(glm::ceil(p_ent->GetComponent<ParticleBufferComponent>()->GetCurrentAllocatedParticles() / 32.f), 1, 1);

	}


	void GameLayer::Update() {

		if (Input::IsKeyDown('h'))
			map_scale *= 1.01;

		if (Input::IsKeyDown('g'))
			map_scale *= 0.99;

		if (Input::IsKeyPressed('j'))
			map_scale = 1.0;

		UpdateParticles();
	}

	void GameLayer::DrawMap() {
		glDepthFunc(GL_ALWAYS);
		glViewport(0, 0, MAP_RESOLUTION, MAP_RESOLUTION);
		p_map_fb->Bind();
		GL_StateManager::DefaultClearBits();
		p_map_shader->ActivateProgram();
		p_map_shader->SetUniform("u_scale", map_scale);
		Renderer::DrawQuad();
		Renderer::GetFramebufferLibrary().UnbindAllFramebuffers();
		glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
		glDepthFunc(GL_LEQUAL);
	}

	void GameLayer::RenderParticles() {
		p_particle_render_shader_variants->Activate(0);

		const auto* p_mat = AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID);
		const auto* p_mesh = AssetManager::GetAsset<MeshAsset>(ORNG_BASE_MESH_ID);

		auto* p_ent = p_editor_layer->m_active_scene->GetEntity("Pbuf");
		if (!p_ent->GetComponent<ParticleBufferComponent>()) {
			ORNG_CORE_TRACE("NOT FOUND");
			return;
		}
		GL_StateManager::BindVAO(p_mesh->GetVAO().GetHandle());

		GL_StateManager::BindSSBO(p_ent->GetComponent<ParticleBufferComponent>()->m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);
		SceneRenderer::DrawMeshGBuffer(p_particle_render_shader_variants, p_mesh, SOLID, p_ent->GetComponent<ParticleBufferComponent>()->GetCurrentAllocatedParticles(), &p_mat);
	}

	void GameLayer::OnRender() {
		//DrawMap();
	}


	void GameLayer::OnShutdown() {}


	void GameLayer::OnImGuiRender() {
		auto pos = SceneRenderer::GetScene()->GetActiveCamera()->GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0];
		std::string coords = std::format("{}, {}, {}", pos.x, pos.y, pos.z);
		std::string scale = std::format("Scale: {}", map_scale);

		if (ImGui::Begin("map", (bool*)0, ImGuiWindowFlags_NoDecoration)) {
			ImGui::Text(coords.c_str());
			ImGui::Text(scale.c_str());
			ImGui::Image(ImTextureID(p_map_tex->GetTextureHandle()), ImVec2(1024, 1024));
		}

		ImGui::End();
	}
}