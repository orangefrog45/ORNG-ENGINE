#include "../headers/GameLayer.h"
#include "../../ORNG-Core/headers/core/CodedAssets.h"

namespace ORNG {

	void GameLayer::OnInit() {
		// Initialize fractal renderpass
		Shader& mb_shader = Renderer::GetShaderLibrary().CreateShader("mandelbulb");
		mb_shader.AddStage(GL_VERTEX_SHADER, ORNG_CORE_LIB_DIR "res/shaders/QuadVS.glsl");
		mb_shader.AddStage(GL_FRAGMENT_SHADER, GAME_BIN_DIR "/res/shaders/FractalFS.glsl");
		mb_shader.Init();

		Renderpass rp;
		rp.func = [](RenderResources res) {
			Shader& mb_shader = Renderer::GetShaderLibrary().GetShader("mandelbulb");
			mb_shader.ActivateProgram();
			Renderer::DrawQuad(); 
			};

		rp.stage = RenderpassStage::POST_GBUFFER;
		rp.name = "fractal";
		SceneRenderer::AttachRenderpassIntercept(rp);
	}


	void GameLayer::Update() {}


	void GameLayer::OnRender() {

	}


	void GameLayer::OnShutdown() {}


	void GameLayer::OnImGuiRender() {}


}