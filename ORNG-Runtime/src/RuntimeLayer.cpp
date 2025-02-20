#include "../headers/RuntimeLayer.h"
#include "assets/AssetManager.h"
#include "scene/SceneSerializer.h"

#include "rendering/renderpasses/DepthPass.h"
#include "rendering/renderpasses/GBufferPass.h"
#include "rendering/renderpasses/LightingPass.h"
#include "rendering/renderpasses/FogPass.h"
#include "rendering/renderpasses/TransparencyPass.h"
#include "rendering/renderpasses/PostProcessPass.h"

namespace ORNG {
	void RuntimeLayer::OnInit() {
		mp_quad_shader = &Renderer::GetShaderLibrary().GetQuadShader();

		Texture2DSpec spec;
		// Setting up the scene display texture
		spec.format = GL_RGBA;
		spec.internal_format = GL_RGBA16F;
		spec.storage_type = GL_FLOAT;
		spec.mag_filter = GL_NEAREST;
		spec.min_filter = GL_NEAREST;
		spec.width = Window::GetWidth();
		spec.height = Window::GetHeight();
		spec.wrap_params = GL_CLAMP_TO_EDGE;

		mp_display_tex = std::make_unique<Texture2D>("Editor scene display");
		mp_display_tex->SetSpec(spec);

		// Adding a resize event listener so the scene display texture scales with the window
		m_window_event_listener.OnEvent = [this](const Events::WindowEvent& t_event) {
			if (t_event.event_type == Events::WindowEvent::EventType::WINDOW_RESIZE) {
				auto spec = mp_display_tex->GetSpec();
				spec.width = t_event.new_window_size.x;
				spec.height = t_event.new_window_size.y;
				mp_display_tex->SetSpec(spec);

				m_render_graph.Reset();
				InitRenderGraph();
			}
			};

		m_scene.AddDefaultSystems();
		Events::EventManager::RegisterListener(m_window_event_listener);
		AssetManager::GetSerializer().LoadAssetsFromProjectPath("./", true);
		m_scene.LoadScene();
		SceneSerializer::DeserializeScene(m_scene, ".\\scene.yml", true);
		m_scene.Start();

		InitRenderGraph();
	}

	void RuntimeLayer::InitRenderGraph() {
		m_render_graph.AddRenderpass<DepthPass>();
		m_render_graph.AddRenderpass<GBufferPass>();
		m_render_graph.AddRenderpass<LightingPass>();
		m_render_graph.AddRenderpass<FogPass>();
		m_render_graph.AddRenderpass<TransparencyPass>();
		m_render_graph.AddRenderpass<PostProcessPass>();
		m_render_graph.SetData("OutCol", &*mp_display_tex);
		m_render_graph.SetData("PPS", &m_scene.post_processing);
		m_render_graph.SetData("Scene", &m_scene);
		m_render_graph.SetData("BloomInCol", &*mp_display_tex);
		m_render_graph.Init();
	}

	void RuntimeLayer::Update() {
		m_scene.Update(FrameTiming::GetTimeStep());
	}

	void RuntimeLayer::OnRender() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		GL_StateManager::DefaultClearBits();
		m_render_graph.Execute();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		mp_quad_shader->ActivateProgram();
		GL_StateManager::BindTexture(GL_TEXTURE_2D, mp_display_tex->GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR);
		Renderer::DrawQuad();
	}

	void RuntimeLayer::OnShutdown() {}

	void RuntimeLayer::OnImGuiRender() {
		m_scene.OnImGuiRender();
	}
}