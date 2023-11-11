#include "RuntimeLayer.h"
#include <GL/glew.h>
#include "assets/AssetManager.h"
#include "scene/SceneSerializer.h"

namespace ORNG {
	void RuntimeLayer::OnInit() {
		mp_quad_shader = &Renderer::GetShaderLibrary().CreateShader("runtime-quad");
		mp_quad_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/QuadVS.glsl");
		mp_quad_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/QuadFS.glsl");
		mp_quad_shader->Init();

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
			if (t_event.event_type == Events::Event::EventType::WINDOW_RESIZE) {
				auto spec = mp_display_tex->GetSpec();
				spec.width = t_event.new_window_size.x;
				spec.height = t_event.new_window_size.y;
				mp_display_tex->SetSpec(spec);
			}
			};

		mp_scene = std::make_unique<Scene>();
		Events::EventManager::RegisterListener(m_window_event_listener);
		AssetManager::LoadAssetsFromProjectPath("./", true);
		mp_scene->LoadScene(".\\scene.yml");
		SceneSerializer::DeserializeScene(*mp_scene, ".\\scene.yml", true);
		mp_scene->OnStart();
	}

	void RuntimeLayer::Update() {
		mp_scene->Update(FrameTiming::GetTimeStep());
	}

	void RuntimeLayer::OnRender() {
		Renderer::GetFramebufferLibrary().UnbindAllFramebuffers();
		GL_StateManager::DefaultClearBits();

		SceneRenderer::SceneRenderingSettings s;
		s.p_output_tex = &*mp_display_tex;
		SceneRenderer::SetActiveScene(&*mp_scene);
		SceneRenderer::RenderScene(s);

		Renderer::GetFramebufferLibrary().UnbindAllFramebuffers();

		mp_quad_shader->ActivateProgram();
		GL_StateManager::BindTexture(GL_TEXTURE_2D, mp_display_tex->GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR);
		Renderer::DrawQuad();
	}

	void RuntimeLayer::OnShutdown() {}

	void RuntimeLayer::OnImGuiRender() {}
}