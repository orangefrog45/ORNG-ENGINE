#include "../headers/GameLayer.h"
#include "../../ORNG-Core/headers/core/CodedAssets.h"
#include "imgui.h"
#include "core/Input.h"
#include "EditorLayer.h"
#include "components/ParticleBufferComponent.h"
#include "assets/AssetManager.h"

#define MAP_RESOLUTION 1024
#define MAX_FRACTAL_MIP 5


namespace ORNG {
	static float map_scale = 1.0;
	constexpr unsigned FRACTAL_ANIMATOR_SSBO_SIZE = InterpolatorV3::GPU_STRUCT_SIZE_BYTES + 2 * sizeof(float) + (InterpolatorV1::GPU_STRUCT_SIZE_BYTES + 2 * sizeof(float)) * 2;

	enum ParticleUpdateShaderVariant {
		UPDATE,
		INITIALIZE,
		STATE_UPDATE,
	};

	enum class FractalShaderVariant {
		FIRST_PASS,
		MID_PASS,
		FULL_RES_PASS
	};

	void GameLayer::OnInit() {
		p_scene = std::make_unique<Scene>();
		p_editor_layer->SetScene(&p_scene);

		// Initialize fractal renderpass
		p_kleinian_shader_mips = &Renderer::GetShaderLibrary().CreateShaderVariants("kleinian");
		p_kleinian_shader_mips->SetPath(GL_COMPUTE_SHADER, "res/shaders/FractalFS.glsl");
		std::vector<std::string> fractal_uniforms = { "CSize", "u_k_factor_1", "u_k_factor_2", "u_num_iters", "u_pixel_zfar_size", "u_pixel_znear_size"};
		p_kleinian_shader_mips->AddVariant((unsigned)FractalShaderVariant::FIRST_PASS, { "FIRST_PASS" }, fractal_uniforms);
		p_kleinian_shader_mips->AddVariant((unsigned)FractalShaderVariant::MID_PASS, {}, fractal_uniforms);

		p_kleinian_shader_full_res = &Renderer::GetShaderLibrary().CreateShader("kleinian");
		p_kleinian_shader_full_res->AddStage(GL_VERTEX_SHADER, "res/shaders/QuadVS.glsl");
		p_kleinian_shader_full_res->AddStage(GL_FRAGMENT_SHADER, "res/shaders/FractalFS.glsl", {"FULL_RES_PASS"});
		p_kleinian_shader_full_res->Init();
		p_kleinian_shader_full_res->AddUniforms(fractal_uniforms);


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
		p_particle_render_shader_variants->AddVariant(0, {"PARTICLE", "PARTICLES_DETACHED", "BILLBOARD"}, gbuffer_uniforms);

		m_fractal_animator_ssbo.Init();
		m_fractal_animator_ssbo.Resize(FRACTAL_ANIMATOR_SSBO_SIZE);
		GL_StateManager::BindSSBO(m_fractal_animator_ssbo.GetHandle(), 8);

		//p_particle_update_shader_variants->Activate(ParticleUpdateShaderVariant::INITIALIZE);
		//GL_StateManager::DispatchCompute(glm::ceil(p_comp->GetCurrentAllocatedParticles() / 32.f), 1, 1);


		Renderpass rp;
		rp.func = [this](RenderResources res) {
			DrawFractal();
			glBindImageTexture(1, p_fractal_depth_chain->GetTextureHandle(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16F);


			p_kleinian_shader_full_res->ActivateProgram();
			p_kleinian_shader_full_res->SetUniform("CSize", fractal_csize);
			p_kleinian_shader_full_res->SetUniform("u_k_factor_1", k_factor_1);
			p_kleinian_shader_full_res->SetUniform("u_k_factor_2", k_factor_2);
			p_kleinian_shader_full_res->SetUniform<unsigned>("u_num_iters", num_iters);
			Renderer::DrawQuad();
			};

		rp.stage = RenderpassStage::POST_GBUFFER;
		rp.name = "fractal";
		SceneRenderer::AttachRenderpassIntercept(rp);

		Renderpass particle_rp;
		particle_rp.func = [this](RenderResources res) {
			RenderParticles();
			};
		particle_rp.stage = RenderpassStage::POST_GBUFFER;
		particle_rp.name = "particle render";

		//SceneRenderer::AttachRenderpassIntercept(particle_rp);

		Texture2DSpec fractal_depth_spec;
		fractal_depth_spec.format = GL_RED;
		fractal_depth_spec.internal_format = GL_R16F;
		fractal_depth_spec.storage_type = GL_FLOAT;
		fractal_depth_spec.width = Window::GetWidth() * 0.5;
		fractal_depth_spec.height = Window::GetHeight() * 0.5;
		fractal_depth_spec.generate_mipmaps = true;
		p_fractal_depth_chain = std::make_unique<Texture2D>( "Fractal depth chain" );
		p_fractal_depth_chain->SetSpec(fractal_depth_spec);

	}

	void GameLayer::DrawFractal() {
		auto* p_cam = p_scene->GetActiveCamera();


		glm::vec2 zfar_size = { p_cam->zFar * tanf(glm::radians(p_cam->fov * 0.5)) * 2 * p_cam->aspect_ratio, p_cam->zFar * tanf(glm::radians(p_cam->fov * 0.5)) * 2};
		glm::vec2 znear_size = { p_cam->zNear * tanf(glm::radians(p_cam->fov * 0.5)) * 2 * p_cam->aspect_ratio, p_cam->zNear * tanf(glm::radians(p_cam->fov * 0.5)) * 2  };


		for (int i = 0; i <= MAX_FRACTAL_MIP; i++) {
			glClearTexImage(p_fractal_depth_chain->GetTextureHandle(), i, GL_RED, GL_FLOAT, nullptr);
		}
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		{
			glm::vec2 dim = { Window::GetWidth() / 2.0 / pow(2, MAX_FRACTAL_MIP), Window::GetHeight() / 2.0 / pow(2, MAX_FRACTAL_MIP) };
			glm::vec2 pixel_zfar_size = { zfar_size.x / dim.x,  zfar_size.y / dim.y };
			glm::vec2 pixel_znear_size = { znear_size.x / dim.x, znear_size.y / dim.y };

			p_kleinian_shader_mips->Activate((unsigned)FractalShaderVariant::FIRST_PASS);
			p_kleinian_shader_mips->SetUniform("CSize", fractal_csize);
			p_kleinian_shader_mips->SetUniform("u_k_factor_1", k_factor_1);
			p_kleinian_shader_mips->SetUniform("u_k_factor_2", k_factor_2);
			p_kleinian_shader_mips->SetUniform("u_pixel_znear_size", pixel_znear_size);
			p_kleinian_shader_mips->SetUniform("u_pixel_zfar_size", pixel_zfar_size);
			p_kleinian_shader_mips->SetUniform<unsigned>("u_num_iters", num_iters);



			glBindImageTexture(0, p_fractal_depth_chain->GetTextureHandle(), MAX_FRACTAL_MIP, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);
			GL_StateManager::DispatchCompute(dim.x / 8.f, dim.y / 8.f, 1);
		}

		p_kleinian_shader_mips->Activate((unsigned)FractalShaderVariant::MID_PASS);
		p_kleinian_shader_mips->SetUniform("CSize", fractal_csize);
		p_kleinian_shader_mips->SetUniform("u_k_factor_1", k_factor_1);
		p_kleinian_shader_mips->SetUniform("u_k_factor_2", k_factor_2);
		p_kleinian_shader_mips->SetUniform<unsigned>("u_num_iters", num_iters);

		for (int mip_layer = MAX_FRACTAL_MIP - 1; mip_layer >= 0; mip_layer--) {
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			glm::vec2 dim = { Window::GetWidth() / 2.0 / pow(2, mip_layer), Window::GetHeight() / 2.0 / pow(2, mip_layer) };
			glm::vec2 pixel_zfar_size = { zfar_size.x / dim.x,  zfar_size.y / dim.y };
			glm::vec2 pixel_znear_size = { znear_size.x / dim.x, znear_size.y / dim.y };

			p_kleinian_shader_mips->SetUniform("u_pixel_znear_size", pixel_znear_size);
			p_kleinian_shader_mips->SetUniform("u_pixel_zfar_size", pixel_zfar_size);

			glBindImageTexture(0, p_fractal_depth_chain->GetTextureHandle(), mip_layer, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);
			glBindImageTexture(1, p_fractal_depth_chain->GetTextureHandle(), mip_layer + 1, GL_FALSE, 0, GL_READ_ONLY, GL_R16F);
			GL_StateManager::DispatchCompute(dim.x / 8.f, dim.y / 8.f, 1);
		}
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);


	}

	void GameLayer::UpdateParticles() {
		auto* p_ent = p_scene->GetEntity("Pbuf");


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

		//UpdateParticles();
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
		const auto* p_mesh = AssetManager::GetAsset<MeshAsset>(ORNG_BASE_QUAD_ID);

		auto* p_ent = p_scene->GetEntity("Pbuf");
		if (!p_ent->GetComponent<ParticleBufferComponent>()) {
			ORNG_CORE_TRACE("NOT FOUND");
			return;
		}
		GL_StateManager::BindVAO(p_mesh->GetVAO().GetHandle());

		GL_StateManager::BindSSBO(p_ent->GetComponent<ParticleBufferComponent>()->m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);
		SceneRenderer::DrawMeshGBuffer(p_particle_render_shader_variants, p_mesh, SOLID, p_ent->GetComponent<ParticleBufferComponent>()->GetCurrentAllocatedParticles(), &p_mat);
	}

	void GameLayer::OnRender() {
		DrawMap();
	}


	void GameLayer::OnShutdown() {}


	void GameLayer::OnImGuiRender() {
		auto pos = SceneRenderer::GetScene()->GetActiveCamera()->GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0];
		std::string coords = std::format("{}, {}, {}", pos.x, pos.y, pos.z);
		std::string scale = std::format("Scale: {}", map_scale);

		ImGui::SetNextWindowPos({ 0, 0 }, ImGuiCond_Once);
		if (ImGui::Begin("map", (bool*)0, ImGuiWindowFlags_NoDecoration)) {
			ImGui::SliderFloat3("Csize", &fractal_csize[0], 0, 10);
			ImGui::SliderFloat("k1", &k_factor_1, 0, 2);
			ImGui::SliderFloat("k2", &k_factor_2, 0, 2);
			ImGui::SliderInt("Iters", &num_iters, 0, 64);

			if (ImGui::Button("Add point")) {
				m_csize_interpolator.AddPoint(m_csize_interpolator.GetNbPoints(), fractal_csize);
				m_k1_interpolator.AddPoint(m_k1_interpolator.GetNbPoints(), k_factor_1);
				m_k2_interpolator.AddPoint(m_k1_interpolator.GetNbPoints(), k_factor_2);

				std::array<std::byte, FRACTAL_ANIMATOR_SSBO_SIZE> bytes;
				std::byte* p_byte = &bytes[0];

				m_csize_interpolator.ConvertSelfToBytes(p_byte);
				m_k1_interpolator.ConvertSelfToBytes(p_byte);
				m_k2_interpolator.ConvertSelfToBytes(p_byte);

				glNamedBufferSubData(m_fractal_animator_ssbo.GetHandle(), 0, FRACTAL_ANIMATOR_SSBO_SIZE, &bytes[0]);
			}

			ImGui::Text(coords.c_str());
			ImGui::Text(scale.c_str());
			//ImGui::Image(ImTextureID(p_map_tex->GetTextureHandle()), ImVec2(1024, 1024));
		}
		ImGui::End();
	}
}