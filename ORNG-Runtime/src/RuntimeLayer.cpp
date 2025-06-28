#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

#include "../headers/RuntimeLayer.h"

#include <components/systems/EnvMapSystem.h>
#include <yaml-cpp/yaml.h>

#include "assets/AssetManager.h"
#include "scene/SceneSerializer.h"

#include "rendering/renderpasses/DepthPass.h"
#include "rendering/renderpasses/GBufferPass.h"
#include "rendering/renderpasses/LightingPass.h"
#include "rendering/renderpasses/FogPass.h"
#include "rendering/renderpasses/TransparencyPass.h"
#include "rendering/renderpasses/PostProcessPass.h"

#include "components/systems/PhysicsSystem.h"
#include "components/ComponentSystems.h"
#include "components/systems/VrSystem.h"


using namespace ORNG;

void RuntimeLayer::OnInit() {
	LoadRuntimeSettings();

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

	if (m_settings.use_vr) {
		mp_vr = std::make_unique<vrlib::VR>();
		try {
			InitVR();
			XrViewConfigurationView view = mp_vr->GetViewConfigurationView(0);
			spec.width = view.recommendedImageRectWidth;
			spec.height = view.recommendedImageRectHeight;
		} catch(std::exception& e) {
			ORNG_CORE_CRITICAL("Failed to initialize VR runtime: {}", e.what());
			BREAKPOINT;
		}
	}

	mp_display_tex = std::make_unique<Texture2D>("Runtime colour");
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

	m_scene.AddSystem(new CameraSystem{ &m_scene }, 0);
	m_scene.AddSystem(new EnvMapSystem{ &m_scene }, 1000);
	m_scene.AddSystem(new AudioSystem{ &m_scene }, 2000);
	m_scene.AddSystem(new PointlightSystem{ &m_scene }, 3000);
	m_scene.AddSystem(new SpotlightSystem{ &m_scene }, 4000);
	m_scene.AddSystem(new ParticleSystem{ &m_scene }, 5000);
	m_scene.AddSystem(new PhysicsSystem{ &m_scene }, 6000);
	m_scene.AddSystem(new TransformHierarchySystem{ &m_scene }, 7000);
	if (m_settings.use_vr) m_scene.AddSystem(new VrSystem{&m_scene, *mp_vr}, 7999);
	m_scene.AddSystem(new ScriptSystem{ &m_scene }, 8000);
	m_scene.AddSystem(new SceneUBOSystem{ &m_scene }, 9000);
	m_scene.AddSystem(new MeshInstancingSystem{ &m_scene }, 10000);
	Events::EventManager::RegisterListener(m_window_event_listener);
	AssetManager::GetSerializer().LoadAssetsFromProjectPath("./", true);
	m_scene.LoadScene();

	InitRenderGraph();
	m_scene.mp_render_graph = &m_render_graph;

	SceneSerializer::DeserializeScene(m_scene, ".\\scene.yml", true);
	m_scene.Start();
}

void RuntimeLayer::InitVR() {
	GLFWwindow* p_window = Window::GetGLFWwindow();
	auto hDC = GetDC(glfwGetWin32Window(p_window));
	auto hGLRC = glfwGetWGLContext(p_window);

	static vrlib::OpenXR_DebugLogFunc log_func = [](const std::string& log, unsigned level) {
		switch(level) {
			case 0:
				ORNG_CORE_TRACE(log)
				break;
			case 1:
				ORNG_CORE_INFO(log);
			break;
			case 2:
				ORNG_CORE_WARN(log);
			break;
			case 3:
				ORNG_CORE_ERROR(log);
			break;
		}
	};

	mp_vr->Init(hDC, hGLRC, log_func, {GL_RGBA16F, GL_RGB16F, GL_RGBA8},
		{GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT16});

	mp_vr_framebuffer = std::make_unique<Framebuffer>();
	mp_vr_framebuffer->Init();
	mp_vr_framebuffer->Bind();
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
	mp_vr_framebuffer->EnableDrawBuffers(1, buffers);
}

void RuntimeLayer::LoadRuntimeSettings() {
	ORNG_CORE_INFO("Loading runtime settings");

	std::string file = ReadTextFile("runtime.orts");
	if (file.empty()) {
		ORNG_CORE_CRITICAL("Failed to read runtime settings, runtime.orts is missing, inaccessible or corrupted.");
		BREAKPOINT;
	}

	YAML::Node node = YAML::Load(file);
	m_settings.use_vr = node["VR"].as<bool>();
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
	if (m_settings.use_vr) {
		mp_vr->PollEvents();
		XrSessionState session_state = mp_vr->GetSessionState();

		if (session_state == XR_SESSION_STATE_EXITING || session_state == XR_SESSION_STATE_LOSS_PENDING ||
			session_state == XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) {
			ORNG_CORE_ERROR("Runtime exited due to VR instance loss or session exit.");
			// TODO: kill runtime
			} else {
				m_xr_frame_state = mp_vr->BeginFrame();
				mp_vr->input.PollActions(m_xr_frame_state.predictedDisplayTime);
			}
	}
	
	m_scene.Update(FrameTiming::GetTimeStep());
}

void RuntimeLayer::RenderToVrTargets() {
	if (!mp_vr->IsSessionRunning()) return;

	vrlib::VR::RenderLayerInfo render_layer_info;
	render_layer_info.predicted_display_time = m_xr_frame_state.predictedDisplayTime;

	if (mp_vr->ShouldRender(m_xr_frame_state)) {
		auto targets = mp_vr->AcquireColourAndDepthRenderTargets(m_xr_frame_state, render_layer_info);

		auto* p_cam = m_scene.GetSystem<ORNG::CameraSystem>().GetActiveCamera();
		auto& vr_system = m_scene.GetSystem<VrSystem>();

		for (size_t i = 0; i < targets.size(); i++) {
			if (vr_system.ShouldUseMatrices()) {
				auto [view, proj] = vr_system.GetEyeMatrices(i);
				m_scene.GetSystem<ORNG::SceneUBOSystem>().UpdateMatrixUBO(&proj, &view);
			}

			m_render_graph.Execute();

			mp_vr_framebuffer->BindTexture2D(targets[i].colour_tex_handle, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
			ORNG::GL_StateManager::BindTexture(GL_TEXTURE_2D, mp_display_tex->GetTextureHandle(), GL_TEXTURE1);
			ORNG::Renderer::GetShaderLibrary().GetQuadShader().ActivateProgram();
			glClearColor(1.f, 0.f, 0.f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glViewport(0, 0, targets[i].resolution[0], targets[i].resolution[1]);
			ORNG::Renderer::DrawQuad();

			mp_vr->ReleaseColourAndDepthRenderTargets(render_layer_info, targets[i]);
		}

		render_layer_info.layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&render_layer_info.layer_projection));
	}

	mp_vr->EndFrame(m_xr_frame_state, render_layer_info);
}

void RuntimeLayer::RenderToPcTarget() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GL_StateManager::DefaultClearBits();
	m_render_graph.Execute();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	mp_quad_shader->ActivateProgram();
	GL_StateManager::BindTexture(GL_TEXTURE_2D, mp_display_tex->GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR);
	Renderer::DrawQuad();
}

void RuntimeLayer::OnRender() {
	 if (m_settings.use_vr)
	 	RenderToVrTargets();
	else
		RenderToPcTarget();

	m_scene.OnRender();
}

void RuntimeLayer::OnShutdown() {}

void RuntimeLayer::OnImGuiRender() {
	m_scene.OnImGuiRender();
}
