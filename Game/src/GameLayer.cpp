#include "../headers/GameLayer.h"
#include "../../ORNG-Core/headers/core/CodedAssets.h"
#include "imgui.h"
#include "core/Input.h"

constexpr unsigned MAP_RESOLUTION = 512;

namespace ORNG {
	static float map_scale = 1.0;

	void GameLayer::OnInit() {
		// Initialize fractal renderpass
		p_kleinian_shader = &Renderer::GetShaderLibrary().CreateShader("kleinian");
		p_kleinian_shader->AddStage(GL_VERTEX_SHADER, ORNG_CORE_LIB_DIR "res/shaders/QuadVS.glsl");
		p_kleinian_shader->AddStage(GL_FRAGMENT_SHADER, GAME_BIN_DIR "/res/shaders/FractalFS.glsl");
		p_kleinian_shader->Init();

		p_map_shader = &Renderer::GetShaderLibrary().CreateShader("kleinian-map");
		p_map_shader->AddStage(GL_VERTEX_SHADER, ORNG_CORE_LIB_DIR "res/shaders/QuadVS.glsl");
		p_map_shader->AddStage(GL_FRAGMENT_SHADER, GAME_BIN_DIR "/res/shaders/FractalMap.glsl");
		p_map_shader->Init();
		p_map_shader->AddUniform("u_scale");

		p_map_fb = &Renderer::GetFramebufferLibrary().CreateFramebuffer("fractal-map", false);
		p_map_tex = std::make_unique<Texture2D>("map");
		Texture2DSpec spec;
		spec.width = 1024;
		spec.height = 1024;
		spec.storage_type = GL_FLOAT;
		spec.internal_format = GL_RGB16F;
		spec.format = GL_RGB;

		p_map_tex->SetSpec(spec);
		p_map_fb->BindTexture2D(p_map_tex->GetTextureHandle(), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);

		Renderpass rp;
		rp.func = [&](RenderResources res) {
			p_kleinian_shader->ActivateProgram();
			Renderer::DrawQuad();
			};

		rp.stage = RenderpassStage::POST_GBUFFER;
		rp.name = "fractal";
		SceneRenderer::AttachRenderpassIntercept(rp);
	}


	void GameLayer::Update() {
		if (Input::IsKeyDown('h'))
			map_scale *= 1.01;

		if (Input::IsKeyDown('g'))
			map_scale *= 0.99;
	}


	void GameLayer::OnRender() {
		glDepthFunc(GL_ALWAYS);
		glViewport(0, 0, 1024, 1024);
		p_map_fb->Bind();
		GL_StateManager::DefaultClearBits();
		p_map_shader->ActivateProgram();
		p_map_shader->SetUniform("u_scale", map_scale);
		Renderer::DrawQuad();
		Renderer::GetFramebufferLibrary().UnbindAllFramebuffers();
		glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
		glDepthFunc(GL_LEQUAL);
	}


	void GameLayer::OnShutdown() {}


	void GameLayer::OnImGuiRender() {
		auto pos = SceneRenderer::GetScene()->GetActiveCamera()->GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0];
		std::string coords = std::format("{}, {}, {}", pos.x, pos.y, pos.z);
		if (ImGui::Begin("map", (bool*)0, ImGuiWindowFlags_NoDecoration)) {
			ImGui::Text(coords.c_str());
			ImGui::Image(ImTextureID(p_map_tex->GetTextureHandle()), ImVec2(1024, 1024));
		}

		ImGui::End();
	}
}