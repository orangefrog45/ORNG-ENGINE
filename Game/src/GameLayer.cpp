#include "../headers/GameLayer.h"
#include "../../ORNG-Core/headers/core/CodedAssets.h"

namespace ORNG {

	void GameLayer::OnInit() {
		// Initialize fractal renderpass
		Shader& mb_shader = Renderer::GetShaderLibrary().CreateShader("mandelbulb");
		mb_shader.AddStageFromString(GL_VERTEX_SHADER, CodedAssets::QuadVS);
		mb_shader.AddStage(GL_FRAGMENT_SHADER, ORNG_CORE_MAIN_DIR "/../ORNG-Editor/res/shaders/MandelbulbFS.glsl");
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