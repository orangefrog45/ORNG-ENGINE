#include "pch/pch.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif defined(MSVC)
#pragma warning( push )
#pragma warning( disable : 4312 ) // 'reinterpret_cast': conversion from 'unsigned int' to 'void *' of greater size
#endif
#include <../extern/Icons.h>
#include <../extern/imgui/backends/imgui_impl_opengl3.h>
#include <fmod.hpp>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <yaml/src/scanscalar.h>
#include <yaml-cpp/yaml.h>
#include <imgui/imgui_internal.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "EditorLayer.h"

#include "components/systems/EnvMapSystem.h"
#include "scene/SceneSerializer.h"
#include "scene/SerializationUtil.h"
#include "util/Timers.h"
#include "assets/AssetManager.h"
#include "scripting/ScriptingEngine.h"
#include "core/Input.h"
#include "util/ExtraUI.h"
#include "components/ParticleBufferComponent.h"
#include "tracy/public/tracy/Tracy.hpp"
#include "components/ComponentSystems.h"
#include "rendering/renderpasses/BloomPass.h"
#include "rendering/renderpasses/DepthPass.h"
#include "rendering/renderpasses/GBufferPass.h"
#include "rendering/renderpasses/LightingPass.h"
#include "rendering/renderpasses/FogPass.h"
#include "rendering/renderpasses/TransparencyPass.h"
#include "rendering/renderpasses/PostProcessPass.h"
#include "rendering/renderpasses/SSAOPass.h"
#include "rendering/renderpasses/VoxelPass.h"
#include "components/systems/PointlightSystem.h"
#include "components/systems/SpotlightSystem.h"
#include "components/systems/SceneUBOSystem.h"

#include "components/PhysicsComponent.h"
#include "components/systems/PhysicsSystem.h"

#include "components/systems/VrSystem.h"
#include "layers/RuntimeSettings.h"

constexpr int RIGHT_WINDOW_WIDTH = 650;
constexpr int BOTTOM_WINDOW_HEIGHT = 300;

using namespace ORNG::Events;

#define SCENE mp_scene_context

using namespace ORNG;

static ImVec2 ImVec2i(int x, int y) {
	return ImVec2{static_cast<float>(x), static_cast<float>(y)};
}

void EditorLayer::Init() {
	char buffer[ORNG_MAX_FILEPATH_SIZE];
	GetModuleFileName(nullptr, buffer, ORNG_MAX_FILEPATH_SIZE);
	m_state.executable_directory = buffer;
	m_state.executable_directory = m_state.executable_directory.substr(0, m_state.executable_directory.find_last_of('\\'));

	InitImGui();
	InitLua();
	m_logger_ui.Init();

	m_res.line_vao.AddBuffer<VertexBufferGL<glm::vec3>>(0, GL_FLOAT, 3, GL_STREAM_DRAW);
	m_res.line_vao.Init();

	m_res.raymarch_shader.SetPath(GL_VERTEX_SHADER, m_state.executable_directory + "/res/core-res/shaders/QuadVS.glsl");
	m_res.raymarch_shader.SetPath(GL_FRAGMENT_SHADER, m_state.executable_directory + "/res/shaders/RaymarchFS.glsl");
	m_res.raymarch_shader.AddVariant(static_cast<unsigned>(EditorResources::RaymarchSV::CAPSULE), { "CAPSULE" },
		{"u_capsule_pos", "u_capsule_height", "u_capsule_radius"});

	m_res.grid_mesh = std::make_unique<GridMesh>();
	m_res.grid_mesh->Init();
	m_res.grid_shader.AddStage(GL_VERTEX_SHADER, m_state.executable_directory + "/res/shaders/GridVS.glsl");
	m_res.grid_shader.AddStage(GL_FRAGMENT_SHADER, m_state.executable_directory + "/res/shaders/GridFS.glsl");
	m_res.grid_shader.Init();

	m_res.quad_col_shader.AddStage(GL_VERTEX_SHADER, m_state.executable_directory + "/res/core-res/shaders/QuadVS.glsl");
	m_res.quad_col_shader.AddStage(GL_FRAGMENT_SHADER, m_state.executable_directory + "/res/core-res/shaders/ColourFS.glsl");
	m_res.quad_col_shader.Init();
	m_res.quad_col_shader.AddUniforms("u_colour");

	m_res.picking_shader.AddStage(GL_VERTEX_SHADER, m_state.executable_directory + "/res/core-res/shaders/TransformVS.glsl");
	m_res.picking_shader.AddStage(GL_FRAGMENT_SHADER, m_state.executable_directory + "/res/shaders/PickingFS.glsl");
	m_res.picking_shader.Init();
	m_res.picking_shader.AddUniforms("comp_id", "transform");

	m_res.highlight_shader.AddStage(GL_VERTEX_SHADER, m_state.executable_directory + "/res/core-res/shaders/TransformVS.glsl", {"OUTLINE"});
	m_res.highlight_shader.AddStage(GL_FRAGMENT_SHADER, m_state.executable_directory + "/res/core-res/shaders/ColourFS.glsl");
	m_res.highlight_shader.Init();
	m_res.highlight_shader.AddUniforms("transform", "u_colour", "u_scale");

	// Setting up the scene display texture
	m_res.colour_render_texture_spec.format = GL_RGBA;
	m_res.colour_render_texture_spec.internal_format = GL_RGBA16F;
	m_res.colour_render_texture_spec.storage_type = GL_FLOAT;
	m_res.colour_render_texture_spec.mag_filter = GL_NEAREST;
	m_res.colour_render_texture_spec.min_filter = GL_NEAREST;
	m_res.colour_render_texture_spec.width = Window::GetWidth();
	m_res.colour_render_texture_spec.height = Window::GetHeight();
	m_res.colour_render_texture_spec.wrap_params = GL_CLAMP_TO_EDGE;

	m_res.p_scene_display_texture = std::make_unique<Texture2D>("Editor scene display");
	m_res.p_scene_display_texture->SetSpec(m_res.colour_render_texture_spec);

	// Adding a resize event listener so the scene display texture scales with the window
	m_window_event_listener.OnEvent = [this](const Events::WindowEvent& t_event) {
		if (t_event.event_type == Events::WindowEvent::EventType::WINDOW_RESIZE) {
			auto spec = m_res.p_scene_display_texture->GetSpec();
			spec.width = t_event.new_window_size.x;
			spec.height = t_event.new_window_size.y;
			m_res.p_scene_display_texture->SetSpec(spec);

			UpdateSceneDisplayRect();

			// Don't change the render graph here as it'll be reinitialized when exiting simulation mode
			if (m_state.use_vr_in_simulation && m_state.simulate_mode_active) return;

			m_render_graph.Reset();
			InitRenderGraph(m_render_graph, false);
		}
		};

	Events::EventManager::RegisterListener(m_window_event_listener);

	m_switch_scene_event_listener.OnEvent = [this](const SwitchSceneEvent& _event) {
		SetActiveScene(*_event.p_new);
	};
	Events::EventManager::RegisterListener(m_switch_scene_event_listener);

	Texture2DSpec picking_spec;
	picking_spec.format = GL_RGB_INTEGER;
	picking_spec.internal_format = GL_RGB32UI;
	picking_spec.storage_type = GL_UNSIGNED_INT;
	picking_spec.width = Window::GetWidth();
	picking_spec.height = Window::GetHeight();

	// Entity ID's are split into halves for storage in textures then recombined later as there is no format for 64 bit uints
	m_res.picking_fb.Init();
	m_res.picking_fb.AddRenderbuffer(Window::GetWidth(), Window::GetHeight());
	m_res.picking_tex.SetSpec(picking_spec);
	m_res.picking_tex.OnResize = [this] {
		m_res.picking_fb.BindTexture2D(m_res.picking_tex.GetTextureHandle(), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
	};
	m_res.picking_tex.OnResize();

	static auto s = &*SCENE;
	m_event_stack.SetContext(s, &m_state.selected_entity_ids);

	m_res.editor_pass_fb.Init();
	//m_res.editor_pass_fb.BindTexture2D(m_scene_renderer.m_gbf_depth.GetTextureHandle(), GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D);
	m_res.editor_pass_fb.BindTexture2D(m_res.p_scene_display_texture->GetTextureHandle(), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
	m_res.editor_pass_fb.AddRenderbuffer(Window::GetWidth(), Window::GetHeight());

	std::string base_proj_dir = m_state.executable_directory + "\\projects\\base-project";
	if (!std::filesystem::exists(base_proj_dir)) {
		GenerateProject("base-project", false);
	}

	MakeProjectActive(m_start_filepath);

	UpdateSceneDisplayRect();

	ORNG_CORE_INFO("Editor layer initialized");
	EventManager::DispatchEvent(EditorEvent(EditorEventType::POST_INITIALIZATION));

	m_asset_manager_window.p_extern_scene = mp_scene_context;
	m_asset_manager_window.Init();
}

void EditorLayer::InitRenderGraph(RenderGraph& graph, bool use_vr) {
	graph.Reset();

	// The path is changed to force shaders to be loaded from the resources folder in the editor binary directory instead of the local project
	// This allows easy shader hot-reloading and modification
	std::string prev_path = std::filesystem::current_path().string();
	std::filesystem::current_path(m_state.executable_directory);

	graph.AddRenderpass<DepthPass>();
	//m_render_graph.AddRenderpass<VoxelPass>();
	graph.AddRenderpass<GBufferPass>();
	graph.AddRenderpass<SSAOPass>();
	graph.AddRenderpass<LightingPass>();
	graph.AddRenderpass<FogPass>();
	graph.AddRenderpass<TransparencyPass>();
	graph.AddRenderpass<BloomPass>();
	graph.AddRenderpass<PostProcessPass>();
	graph.SetData("OutCol", use_vr ? &*m_res.p_vr_scene_display_texture : &*m_res.p_scene_display_texture);
	graph.SetData("BloomInCol", use_vr ? &*m_res.p_vr_scene_display_texture : &*m_res.p_scene_display_texture);
	graph.SetData("PPS", &SCENE->post_processing);
	graph.SetData("Scene", SCENE);
	graph.Init();
	std::filesystem::current_path(prev_path);
}

void EditorLayer::UpdateSceneDisplayRect() {
	if (m_state.fullscreen_scene_display)
		m_state.scene_display_rect = { ImVec2(Window::GetWidth(), Window::GetHeight() - m_res.toolbar_height) };
	else
		m_state.scene_display_rect = { ImVec2(Window::GetWidth() - RIGHT_WINDOW_WIDTH, Window::GetHeight() - BOTTOM_WINDOW_HEIGHT - m_res.toolbar_height) };

	mp_editor_camera->GetComponent<CameraComponent>()->aspect_ratio = m_state.scene_display_rect.x / m_state.scene_display_rect.y;
}


void EditorLayer::BeginPlayScene() {
	ORNG_TRACY_PROFILE;
	SceneSerializer::SerializeScene(*SCENE, m_state.temp_scene_serialization, true);
	m_state.simulate_mode_active = true;

	// Set to fullscreen so mouse coordinate and gui operations in scripts work correctly as they would in a runtime layer
	m_state.fullscreen_scene_display = true;

	if (m_state.use_vr_in_simulation) {
		InitVrForSimulationMode();
		m_state.p_vr_render_graph = std::make_unique<RenderGraph>();
		InitRenderGraph(*m_state.p_vr_render_graph, true);
	}

	SCENE->mp_render_graph = &m_render_graph;
	UpdateSceneDisplayRect();
	SCENE->Start();
	EventManager::DispatchEvent(EditorEvent(EditorEventType::SCENE_START_SIMULATION));
}

void EditorLayer::EndPlayScene() {
	ORNG_TRACY_PROFILE;

	SCENE->m_time_elapsed = 0.0;

	// Reset mouse state as scripts may have modified it
	Window::SetCursorVisible(true);

	m_state.selected_entity_ids.clear();

	auto* p_cam_transform = mp_editor_camera->GetComponent<TransformComponent>();
	const glm::vec3 cam_pos = p_cam_transform->GetAbsPosition();
	const glm::vec3 look_at_pos = cam_pos + p_cam_transform->forward;

	mp_editor_camera = nullptr;

	SCENE->ClearAllEntities();
	SceneSerializer::DeserializeScene(*SCENE, m_state.temp_scene_serialization, false);

	// Reset render graph in case scripts have changed it
	RenderGraph& render_graph = m_state.use_vr_in_simulation ? *m_state.p_vr_render_graph.get() : m_render_graph;
	render_graph.Reset();
	InitRenderGraph(render_graph, false);

	mp_editor_camera = std::make_unique<SceneEntity>(&*SCENE, SCENE->m_registry.create(), &SCENE->m_registry, SCENE->m_static_uuid());
	auto* p_transform = mp_editor_camera->AddComponent<TransformComponent>();
	p_transform->SetAbsolutePosition(cam_pos);
	p_transform->LookAt(look_at_pos);
	mp_editor_camera->AddComponent<CameraComponent>()->MakeActive();

	m_state.simulate_mode_active = false;
	m_state.fullscreen_scene_display = false;

	UpdateSceneDisplayRect();

	if (m_state.use_vr_in_simulation)
		ShutdownVrForSimulationMode();

	SCENE->End();
	EventManager::DispatchEvent(EditorEvent(EditorEventType::SCENE_END_SIMULATION));
}

void EditorLayer::SetVrMode(bool use_vr) {
	m_state.use_vr_in_simulation = use_vr;
}

void EditorLayer::InitVrForSimulationMode() {
	m_state.p_vr = std::make_unique<vrlib::VR>();
	auto* p_window = ORNG::Window::GetGLFWwindow();
	auto hDC = GetDC(glfwGetWin32Window(p_window));
	auto hGLRC = glfwGetWGLContext(p_window);

	static vrlib::OpenXR_DebugLogFunc log_func = [](const std::string& log, unsigned level) {
		switch(level) {
			case 0:
				ORNG_CORE_TRACE(log);
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
			default:
				BREAKPOINT;
		}
	};

	try {
		m_state.p_vr->Init(hDC, hGLRC, log_func, {GL_RGBA16F, GL_RGB16F, GL_RGBA8},
			{GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT16});

		m_res.vr_colour_render_texture_spec.format = GL_RGBA;
		m_res.vr_colour_render_texture_spec.internal_format = GL_RGBA16F;
		m_res.vr_colour_render_texture_spec.storage_type = GL_FLOAT;
		m_res.vr_colour_render_texture_spec.mag_filter = GL_NEAREST;
		m_res.vr_colour_render_texture_spec.min_filter = GL_NEAREST;
		XrViewConfigurationView view = m_state.p_vr->GetViewConfigurationView(0);
		m_res.vr_colour_render_texture_spec.width = static_cast<int>(view.recommendedImageRectWidth);
		m_res.vr_colour_render_texture_spec.height = static_cast<int>(view.recommendedImageRectHeight);
		m_res.vr_colour_render_texture_spec.wrap_params = GL_CLAMP_TO_EDGE;
		m_res.p_vr_scene_display_texture = std::make_unique<Texture2D>("");
		m_res.p_vr_scene_display_texture->SetSpec(m_res.vr_colour_render_texture_spec);

		SCENE->AddSystem(new VrSystem(SCENE, *m_state.p_vr), 7999);

		m_state.p_vr_framebuffer = std::make_unique<Framebuffer>();
		m_state.p_vr_framebuffer->Init();
		m_state.p_vr_framebuffer->Bind();
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
		m_state.p_vr_framebuffer->EnableDrawBuffers(1, buffers);
		SCENE->mp_render_graph = m_state.p_vr_render_graph.get();
	} catch (std::exception& e) {
		ORNG_CORE_ERROR("Failed to initialize VRlib, make sure your VR device is accessible to this device.");
		ORNG_CORE_ERROR("VRlib error: {}", e.what());
		SetVrMode(false);
		ShutdownVrForSimulationMode();
	}
}

void EditorLayer::ShutdownVrForSimulationMode() {
	if (SCENE->HasSystem<VrSystem>()) SCENE->RemoveSystem<VrSystem>();
	m_state.p_vr->Shutdown();
	m_state.p_vr = nullptr;
	m_res.p_vr_scene_display_texture = nullptr;
	m_state.p_vr_framebuffer = nullptr;
	m_state.p_vr_render_graph->Reset();
	m_state.p_vr_render_graph = nullptr;
	SCENE->mp_render_graph = &m_render_graph;
}


void EditorLayer::OnShutdown() {
	if (m_state.simulate_mode_active)
		EndPlayScene();

	mp_editor_camera = nullptr;

	SCENE->UnloadScene();
	m_asset_manager_window.OnShutdown();
	m_logger_ui.Shutdown();
}

void EditorLayer::InitImGui() {
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	io.Fonts->AddFontDefault();
	io.FontDefault = io.Fonts->AddFontFromFileTTF(".\\res\\fonts\\Uniform.ttf", 16.0f);

	ImFontConfig config;
	config.MergeMode = true;
	static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	io.Fonts->AddFontFromFileTTF("./res/fonts/fa-regular-400.ttf", 18.0f, &config, icon_ranges);
	io.Fonts->AddFontFromFileTTF("./res/fonts/fa-solid-900.ttf", 18.0f, &config, icon_ranges);

	m_res.p_xl_font = io.Fonts->AddFontFromFileTTF(".\\res\\fonts\\Uniform.ttf", 36.0f);
	m_res.p_l_font = io.Fonts->AddFontFromFileTTF(".\\res\\fonts\\Uniform.ttf", 26.0f);
	m_res.p_m_font = io.Fonts->AddFontFromFileTTF(".\\res\\fonts\\Uniform.ttf", 20.0f);

	ImGui_ImplOpenGL3_CreateFontsTexture();
	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_Button] = m_res.lighter_grey_color;
	style.Colors[ImGuiCol_ButtonHovered] = m_res.lightest_grey_color;
	style.Colors[ImGuiCol_ButtonActive] = m_res.lightest_grey_color;
	style.Colors[ImGuiCol_Border] = m_res.lighter_grey_color;
	style.Colors[ImGuiCol_Tab] = m_res.lighter_grey_color;
	style.Colors[ImGuiCol_TabHovered] = m_res.lightest_grey_color;
	style.Colors[ImGuiCol_TabActive] = m_res.orange_color;
	style.Colors[ImGuiCol_Header] = m_res.lighter_grey_color;
	style.Colors[ImGuiCol_HeaderHovered] = m_res.lightest_grey_color;
	style.Colors[ImGuiCol_HeaderActive] = m_res.orange_color;
	style.Colors[ImGuiCol_FrameBg] = m_res.lighter_grey_color;
	style.Colors[ImGuiCol_FrameBgHovered] = m_res.lighter_grey_color;
	style.Colors[ImGuiCol_FrameBgActive] = m_res.lightest_grey_color;
	style.Colors[ImGuiCol_WindowBg] = m_res.dark_grey_color;
	style.Colors[ImGuiCol_Border] = m_res.lighter_grey_color;
	style.Colors[ImGuiCol_TitleBg] = m_res.lighter_grey_color;
	style.Colors[ImGuiCol_TitleBgActive] = m_res.lighter_grey_color;
}

void EditorLayer::PollKeybinds() {
	if (!ImGui::GetIO().WantCaptureKeyboard && !Window::Get().input.IsMouseDown(1)) {
		// Tab - Show/hide ui in simulation mode
		if (Window::Get().input.IsKeyPressed(Key::Tab)) {
			m_state.render_ui_in_simulation = !m_state.render_ui_in_simulation;
		}

		if (m_state.selection_mode == SelectionMode::ENTITY) {
			// Ctrl+D - Duplicate entity
			if (Window::Get().input.IsKeyDown(Key::LeftControl) && Window::Get().input.IsKeyPressed(Key::D)) {
				auto vec = DuplicateEntitiesTracked(m_state.selected_entity_ids);
				std::vector<uint64_t> duplicate_ids;
				for (auto* p_ent : vec) {
					duplicate_ids.push_back(p_ent->GetUUID());
				}
				m_state.selected_entity_ids = duplicate_ids;
			}

			// Ctrl+x - Delete entity
			if (Window::Get().input.IsKeyDown(Key::LeftControl) && Window::Get().input.IsKeyPressed(Key::X)) {
				DeleteEntitiesTracked(m_state.selected_entity_ids);
			}
		}

		// Ctrl+z / Ctrl+shift+z undo/redo
		if (Window::Get().input.IsKeyDown(Key::LeftControl) && Window::Get().input.IsKeyPressed('z') && !m_state.simulate_mode_active) { // Undo/redo disabled in simulation mode to prevent overlap during (de)serialization switching back and forth
			if (Window::Get().input.IsKeyDown(Key::Shift))
				m_event_stack.Redo();
			else
				m_event_stack.Undo();
		}

		// K - Make editor cam active
		if (Window::Get().input.IsKeyDown(Key::K))
			mp_editor_camera->GetComponent<CameraComponent>()->MakeActive();

		// F - Focus on entity
		if (Window::Get().input.IsKeyDown(Key::F) && !m_state.selected_entity_ids.empty()) {
			auto* p_entity_transform = SCENE->GetEntity(m_state.selected_entity_ids[0])->GetComponent<TransformComponent>();
			auto* p_editor_cam_transform = mp_editor_camera->GetComponent<TransformComponent>();
			auto pos = p_entity_transform->GetAbsPosition();
			auto offset = pos - p_editor_cam_transform->GetAbsPosition();

			if (length(offset) < 0.001f)
				offset = { 1, 0, 0 };

			p_editor_cam_transform->SetAbsolutePosition(pos - glm::normalize(offset) * 3.f);
			p_editor_cam_transform->LookAt(pos);
		}

		// Escape - exit simulation mode
		if (m_state.simulate_mode_active && Window::Get().input.IsKeyDown(Key::Escape)) {
			EndPlayScene();
		}
	}

	// Drag state tracking
	static bool dragging = false;
	if (Window::Get().input.IsMouseClicked(0)) {
		m_state.mouse_drag_data.start = Window::Get().input.GetMousePos();
		if (!m_state.fullscreen_scene_display) m_state.mouse_drag_data.start = ConvertFullscreenMouseToDisplayMouse(m_state.mouse_drag_data.start);
	}

	if (dragging) {
		m_state.mouse_drag_data.end = Window::Get().input.GetMousePos();
		if (!m_state.fullscreen_scene_display) m_state.mouse_drag_data.end = ConvertFullscreenMouseToDisplayMouse(m_state.mouse_drag_data.end);
		MultiSelectDisplay();
	}

	dragging = ImGui::IsMouseDragging(0) && !ImGui::GetIO().WantCaptureMouse;
}


void EditorLayer::Update() {
	ORNG_TRACY_PROFILE;

	ORNG_PROFILE_FUNC();
	m_state.item_selected_this_frame = false;

	PollKeybinds();

	UpdateEditorCam();

	if (m_state.simulate_mode_active && !m_state.simulate_mode_paused) {
		// Update VR if it's active
		if (m_state.use_vr_in_simulation) {
			m_state.p_vr->PollEvents();
			XrSessionState session_state = m_state.p_vr->GetSessionState();

			if (session_state == XR_SESSION_STATE_EXITING || session_state == XR_SESSION_STATE_LOSS_PENDING) {
				EndPlayScene();
				SetVrMode(false);
				ORNG_CORE_ERROR("VR mode exited due to instance loss or session exit.");
			} else {
				m_state.xr_frame_state = m_state.p_vr->BeginFrame();
				m_state.p_vr->input.PollActions(m_state.xr_frame_state.predictedDisplayTime);
			}
		}

		SCENE->Update(FrameTiming::GetTimeStep());
	}
	else {
		SCENE->GetSystem<SpotlightSystem>().OnUpdate();
		SCENE->GetSystem<PointlightSystem>().OnUpdate();
		SCENE->GetSystem<SceneUBOSystem>().OnUpdate();
		SCENE->GetSystem<MeshInstancingSystem>().OnUpdate(); // This still needs to update so meshes are rendered correctly in the editor
		SCENE->GetSystem<ParticleSystem>().OnUpdate(); // Continue simulating particles for visual feedback
		SCENE->GetSystem<AudioSystem>().OnUpdate(); // For accurate audio playback
		//SCENE->terrain.UpdateTerrainQuadtree(SCENE->m_camera_system.GetActiveCamera()->GetEntity()->GetComponent<TransformComponent>()->GetPosition()); // Needed for terrain LOD updates
	}
}

void EditorLayer::OnRender() {
	RenderDisplayWindow();

	if (m_state.use_vr_in_simulation && m_state.simulate_mode_active) {
		RenderToVrTargets();
	}

	if (m_state.simulate_mode_active)
		mp_scene_context->OnRender();
};

void EditorLayer::RenderToVrTargets() {
	if (!m_state.p_vr->IsSessionRunning()) return;

	vrlib::VR::RenderLayerInfo render_layer_info;
	render_layer_info.predicted_display_time = m_state.xr_frame_state.predictedDisplayTime;

	if (m_state.p_vr->ShouldRender(m_state.xr_frame_state)) {
		auto targets = m_state.p_vr->AcquireColourAndDepthRenderTargets(m_state.xr_frame_state, render_layer_info);

		auto& vr_system = SCENE->GetSystem<VrSystem>();

		for (size_t i = 0; i < targets.size(); i++) {
			if (vr_system.ShouldUseMatrices()) {
				auto [view, proj] = vr_system.GetEyeMatrices(static_cast<unsigned>(i));
				SCENE->GetSystem<ORNG::SceneUBOSystem>().UpdateMatrixUBO(&proj, &view);
			}

            m_state.p_vr_render_graph->Execute();

            m_state.p_vr_framebuffer->BindTexture2D(targets[i].colour_tex_handle, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
            ORNG::GL_StateManager::BindTexture(GL_TEXTURE_2D, m_res.p_vr_scene_display_texture->GetTextureHandle(), GL_TEXTURE1);
            ORNG::Renderer::GetShaderLibrary().GetQuadShader().ActivateProgram();
            glClearColor(1.f, 0.f, 0.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glViewport(0, 0, static_cast<int>(targets[i].resolution[0]), static_cast<int>(targets[i].resolution[1]));
            ORNG::Renderer::DrawQuad();

			m_state.p_vr->ReleaseColourAndDepthRenderTargets(render_layer_info, targets[i]);
		}

		render_layer_info.layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&render_layer_info.layer_projection));
	}

	m_state.p_vr->EndFrame(m_state.xr_frame_state, render_layer_info);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

	void EditorLayer::MultiSelectDisplay() {
	m_state.selected_entity_ids.clear();

	glm::vec2 min = { glm::min(m_state.mouse_drag_data.start.x,  m_state.mouse_drag_data.end.x), glm::min(Window::GetHeight() - m_state.mouse_drag_data.start.y,  Window::GetHeight() - m_state.mouse_drag_data.end.y) };
	glm::vec2 max = { glm::max(m_state.mouse_drag_data.start.x,  m_state.mouse_drag_data.end.x), glm::max(Window::GetHeight() - m_state.mouse_drag_data.start.y, Window::GetHeight() - m_state.mouse_drag_data.end.y) };
	glm::vec2 n = glm::vec2(m_state.scene_display_rect.x, m_state.scene_display_rect.y);

	auto* p_cam = SCENE->GetSystem<CameraSystem>().GetActiveCamera();
	auto* p_transform = p_cam->GetEntity()->GetComponent<TransformComponent>();
	auto pos = p_transform->GetAbsPosition();

	glm::vec3 min_dir = ExtraMath::ScreenCoordsToRayDir(p_cam->GetProjectionMatrix(), min, pos,
		p_transform->forward, p_transform->up, Window::GetWidth(), Window::GetHeight());

	glm::vec3 max_dir = ExtraMath::ScreenCoordsToRayDir(p_cam->GetProjectionMatrix(), max, pos,
		p_transform->forward, p_transform->up, Window::GetWidth(), Window::GetHeight());

	glm::vec3 near_min = pos + min_dir * p_cam->zNear;

	glm::vec3 far_min = pos + min_dir * p_cam->zFar;

	glm::vec3 near_max = pos + max_dir * p_cam->zNear;

	glm::vec3 far_max = pos + max_dir * p_cam->zFar;

	glm::vec3 far_middle = (glm::vec3(far_max) + glm::vec3(far_min)) * 0.5f;
	glm::vec3 near_middle = (glm::vec3(near_max) + glm::vec3(near_min)) * 0.5f;
	glm::vec3 target = glm::normalize(far_middle - near_middle);
	glm::vec3 right = p_transform->right;

	glm::vec3 up = glm::normalize(glm::cross(right, target));

	ExtraMath::Plane t = { glm::cross(right, pos - far_max), pos };

	ExtraMath::Plane b = { glm::cross(right, far_min - pos), pos };

	ExtraMath::Plane l = { glm::cross(up, pos - far_min), pos };

	ExtraMath::Plane r = { glm::cross(up, far_max - pos), pos };

	ExtraMath::Plane ne = { target, pos + target * p_cam->zNear };

	ExtraMath::Plane f{ -target, far_middle };

	for (auto [entity, transform] : SCENE->m_registry.view<TransformComponent>().each()) {
		auto pos1 = transform.GetAbsPosition();
		if (ne.GetSignedDistanceToPlane(pos1) >= 0 &&
			f.GetSignedDistanceToPlane(pos1) >= 0 &&
			r.GetSignedDistanceToPlane(pos1) >= 0 &&
			l.GetSignedDistanceToPlane(pos1) >= 0 &&
			t.GetSignedDistanceToPlane(pos1) >= 0 &&
			b.GetSignedDistanceToPlane(pos1) >= 0
			) {
			auto* p_entity = SCENE->GetEntity(entity);
			if (m_state.general_settings.selection_settings.select_only_parents && p_entity->GetParent() != entt::null)
				continue;

			if (!m_state.general_settings.selection_settings.select_all) {
				if (m_state.general_settings.selection_settings.select_light_objects && (p_entity->HasComponent<PointLightComponent>() || p_entity->HasComponent<SpotLightComponent>()))
					SelectEntity(p_entity->GetUUID());
				else if (m_state.general_settings.selection_settings.select_mesh_objects && p_entity->HasComponent<MeshComponent>())
					SelectEntity(p_entity->GetUUID());
				else if (m_state.general_settings.selection_settings.select_physics_objects && p_entity->HasComponent<PhysicsComponent>())
					SelectEntity(p_entity->GetUUID());
			}
			else {
				SelectEntity(p_entity->GetUUID());
			}
		}
	}
}

void EditorLayer::UpdateEditorCam() {
	if (SCENE->GetSystem<CameraSystem>().GetActiveCamera() != mp_editor_camera->GetComponent<CameraComponent>())
		return;

	static float cam_speed = 0.01f;
	auto* p_cam = mp_editor_camera->GetComponent<CameraComponent>();
	auto* p_transform = mp_editor_camera->GetComponent<TransformComponent>();
	p_cam->aspect_ratio = m_state.scene_display_rect.x / m_state.scene_display_rect.y;
	// Camera movement
	if (ImGui::IsMouseDown(1)) {
		glm::vec3 pos = p_transform->GetAbsPosition();
		glm::vec3 movement_vec{ 0.0, 0.0, 0.0 };
		float time_elapsed = FrameTiming::GetTimeStep();
		movement_vec += p_transform->right * static_cast<float>(Window::Get().input.IsKeyDown(Key::D)) * time_elapsed * cam_speed;
		movement_vec -= p_transform->right * static_cast<float>(Window::Get().input.IsKeyDown(Key::A)) * time_elapsed * cam_speed;
		movement_vec += p_transform->forward * static_cast<float>(Window::Get().input.IsKeyDown(Key::W)) * time_elapsed * cam_speed;
		movement_vec -= p_transform->forward * static_cast<float>(Window::Get().input.IsKeyDown(Key::S)) * time_elapsed * cam_speed;
		movement_vec += glm::vec3(0, 1, 0) * static_cast<float>(Window::Get().input.IsKeyDown(Key::E)) * time_elapsed * cam_speed;
		movement_vec -= glm::vec3(0, 1, 0) * static_cast<float>(Window::Get().input.IsKeyDown(Key::Q)) * time_elapsed * cam_speed;

		if (Window::Get().input.IsKeyDown(Key::Space))
			movement_vec *= 10.0f;

		if (Window::Get().input.IsKeyDown(Key::Shift))
			movement_vec *= 100.0f;

		if (Window::Get().input.IsKeyDown(Key::LeftControl))
			movement_vec *= 0.1f;

		p_transform->SetAbsolutePosition(pos + movement_vec);
	}

	// Camera rotation
	static glm::vec2 last_mouse_pos;
	if (ImGui::IsMouseClicked(1))
		last_mouse_pos = Window::Get().input.GetMousePos();

	if (ImGui::IsMouseDown(1)) {
		float rotation_speed = 0.005f;
		glm::vec2 mouse_coords = Window::Get().input.GetMousePos();
		glm::vec2 mouse_delta = -glm::vec2(mouse_coords.x - last_mouse_pos.x, mouse_coords.y - last_mouse_pos.y);

		glm::vec3 rot_x = glm::rotate(mouse_delta.x * rotation_speed, glm::vec3(0.0, 1.0, 0.0)) * glm::vec4(p_transform->forward, 0);
		glm::fvec3 rot_y = glm::rotate(mouse_delta.y * rotation_speed, p_transform->right) * glm::vec4(rot_x, 0);


		if (rot_y.y <= 0.997f && rot_y.y >= -0.997f)
			p_transform->LookAt(p_transform->GetAbsPosition() + glm::normalize(rot_y));
		else
			p_transform->LookAt(p_transform->GetAbsPosition() + glm::normalize(rot_x));


		Window::SetCursorPos(static_cast<int>(last_mouse_pos.x), static_cast<int>(last_mouse_pos.y));
	}

	p_cam->UpdateFrustum();

	// Update camera pos variable for lua console
	auto pos = mp_editor_camera->GetComponent<TransformComponent>()->GetPosition();
	m_lua_cli.GetLua().script(std::format("pos = vec3.new({}, {}, {})", pos.x, pos.y, pos.z));
}





void EditorLayer::RenderSceneDisplayPanel() {
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::SetNextWindowPos(AddImVec2(ImGui::GetMainViewport()->Pos, ImVec2(0, m_res.toolbar_height)));
	ImGui::SetNextWindowSize(m_state.scene_display_rect);

	if (ImGui::Begin("Scene display overlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | (ImGui::IsMouseDragging(0) ? 0 : ImGuiWindowFlags_NoInputs) | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground)) {
		ImVec2 prev_curs_pos = ImGui::GetCursorPos();
		ImGui::Image(reinterpret_cast<void*>(m_res.p_scene_display_texture->GetTextureHandle()),
			ImVec2{m_state.scene_display_rect.x, m_state.scene_display_rect.y}, ImVec2(0, 1), ImVec2(1, 0));
		//ImGui::Image((ImTextureID)m_render_graph.GetRenderpass<SSAOPass>()->GetSSAOTex().GetTextureHandle(), ImVec2(m_state.scene_display_rect.x, m_state.scene_display_rect.y), ImVec2(0, 1), ImVec2(1, 0));
		ImGui::SetCursorPos(prev_curs_pos);
		ImGui::Dummy(ImVec2(0, 5));
		ImGui::Dummy(ImVec2(5, 0));
		ImGui::SameLine();
		ImGui::InvisibleButton("##drag-drop-scene-target", ImVec2(m_state.scene_display_rect.x - 20, m_state.scene_display_rect.y - 20));

		if (ImGui::BeginDragDropTarget()) {
			if (ImGui::AcceptDragDropPayload("ENTITY")) {
				/* Functionality handled externally */
			}
			else if (const ImGuiPayload* p_mesh_payload = ImGui::AcceptDragDropPayload("MESH")) {
				if (p_mesh_payload->DataSize == sizeof(MeshAsset*)) {
					auto& ent = CreateEntityTracked("Mesh");
					ent.AddComponent<MeshComponent>(*static_cast<MeshAsset**>(p_mesh_payload->Data));
					auto* p_cam_transform = mp_editor_camera->GetComponent<TransformComponent>();
					auto& mesh_aabb = ent.GetComponent<MeshComponent>()->GetMeshData()->m_aabb;
					ent.GetComponent<TransformComponent>()->SetAbsolutePosition(p_cam_transform->GetAbsPosition() + p_cam_transform->forward * (glm::max(glm::max(mesh_aabb.extents.x, mesh_aabb.extents.y), mesh_aabb.extents.z) + 5.f));
					SelectEntity(ent.GetUUID());
				}
			}
			else if (const ImGuiPayload* p_prefab_payload = ImGui::AcceptDragDropPayload("PREFAB")) {
				if (p_prefab_payload->DataSize == sizeof(Prefab*)) {
					Prefab* prefab_data = (*static_cast<Prefab**>(p_prefab_payload->Data));
					auto& ent = m_state.simulate_mode_active ? SCENE->InstantiatePrefab(*prefab_data) : SCENE->InstantiatePrefab(*prefab_data, false);
					auto* p_cam_transform = mp_editor_camera->GetComponent<TransformComponent>();
					glm::vec3 pos;
					if (auto* p_mesh = ent.GetComponent<MeshComponent>()) {
						auto& mesh_aabb = p_mesh->GetMeshData()->m_aabb;
						pos = p_cam_transform->GetAbsPosition() + p_cam_transform->forward * (glm::max(glm::max(mesh_aabb.extents.x, mesh_aabb.extents.y), mesh_aabb.extents.z) + 5.f);
					}
					else {
						pos = p_cam_transform->GetAbsPosition() + p_cam_transform->forward * 5.f;
					}
					ent.GetComponent<TransformComponent>()->SetAbsolutePosition(pos);
				}
			}
			ImGui::EndDragDropTarget();
		}
	}
	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
}


void EditorLayer::RenderUI() {
	if (m_state.simulate_mode_active) {
		SCENE->OnImGuiRender();

		if (!m_state.render_ui_in_simulation) {
			RenderToolbar();
			return;
		}
	}

	if (!m_state.simulate_mode_active)
		RenderSceneDisplayPanel();

	RenderToolbar();
	RenderBottomWindow();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 5);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));

	ImGui::SetNextWindowPos(AddImVec2(ImGui::GetMainViewport()->Pos,
		ImVec2{static_cast<float>(Window::GetWidth() - RIGHT_WINDOW_WIDTH), static_cast<float>(m_res.toolbar_height)}));

	ImGui::SetNextWindowSize(ImVec2i(RIGHT_WINDOW_WIDTH, Window::GetHeight() - m_res.toolbar_height));
	if (ImGui::Begin("##right window", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration)) {
		RenderSceneGraph();
		DisplayEntityEditor();
	}
	ImGui::End();

	RenderGeneralSettingsMenu();

	if (m_state.show_build_menu) RenderBuildMenu();

	ImGui::PopStyleVar(); // window border size
	ImGui::PopStyleVar(); // window padding
}


void EditorLayer::RenderBottomWindow() {
	int window_width = Window::GetWidth() - 650;
	ImGui::SetNextWindowSize(ImVec2{static_cast<float>(window_width), static_cast<float>(BOTTOM_WINDOW_HEIGHT)});
	ImGui::SetNextWindowPos(AddImVec2(ImGui::GetMainViewport()->Pos, ImVec2(0.f, static_cast<float>(Window::GetHeight() - BOTTOM_WINDOW_HEIGHT))));

	ImGui::Begin("##bottom", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration);
	ImGui::BeginTabBar("#log-asset-selector");

	if (ImGui::BeginTabItem("Console")) {
		RenderConsole();
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Assets")) {
		m_asset_manager_window.OnRenderUI();
		ImGui::EndTabItem();
	}

	ImGui::EndTabBar();
	ImGui::End();
}

void EditorLayer::RenderConsole() {
	static unsigned last_num_logs_rendered = 0;

	ImGui::SetWindowFontScale(0.85f);

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f });
	if (ImGui::BeginChild("Logs", {ImGui::GetContentRegionMax().x, ImGui::GetContentRegionMax().y - 60 }, true)) {
		unsigned current_num_logs_rendered = m_logger_ui.RenderLogContentsWithImGui();
		// If scroll is already close to the bottom, make sure it stays locked to the bottom as more logs are added
		if (current_num_logs_rendered != last_num_logs_rendered && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 100.f)
			ImGui::SetScrollHereY(1.f);

		last_num_logs_rendered = current_num_logs_rendered;

	}
	ImGui::EndChild();

	ImGui::SetWindowFontScale(1.f);
	static std::string lua_cmd = "";
	ImGui::Text("> ");
	ImGui::SameLine();
	ImGui::PushItemWidth(ImGui::GetContentRegionMax().x);
	ImGui::InputText("##cmd-input", &lua_cmd);
	ImGui::PopItemWidth();
	if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)) {
		auto result = m_lua_cli.Execute(lua_cmd);
		m_logger_ui.AddLog("> " + result.content, LogEvent::Type::L_TRACE);
		if (result.is_error) {
			m_logger_ui.AddLog(result.content, LogEvent::Type::L_ERROR);
		}

		lua_cmd.clear();
	}

	ImGui::PopStyleColor();
}

void EditorLayer::RenderDisplayWindow() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GL_StateManager::DefaultClearBits();

	if (ImGui::IsMouseClicked(0) && !ImGui::GetIO().WantCaptureMouse) {
		DoPickingPass();
	}
	glPolygonMode(GL_FRONT_AND_BACK, m_state.general_settings.debug_render_settings.render_wireframe ? GL_LINE : GL_FILL);

	m_render_graph.Execute();
	m_asset_manager_window.OnMainRender();

	m_res.editor_pass_fb.Bind();
	m_res.editor_pass_fb.BindTexture2D(m_res.p_scene_display_texture->GetTextureHandle(), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
	// Mouse drag selection quad
	if (ImGui::IsMouseDragging(0) && !ImGui::GetIO().WantCaptureMouse) {
		glDisable(GL_DEPTH_TEST);
		m_res.quad_col_shader.ActivateProgram();
		m_res.quad_col_shader.SetUniform("u_colour", glm::vec4(0, 0, 1, 0.1));
		glm::vec2 w = { Window::GetWidth(), Window::GetHeight() };
		Renderer::DrawScaledQuad((glm::vec2(m_state.mouse_drag_data.start.x, Window::GetHeight() - m_state.mouse_drag_data.start.y) / w) * 2.f - 1.f, (glm::vec2(m_state.mouse_drag_data.end.x, Window::GetHeight() - m_state.mouse_drag_data.end.y) / w) * 2.f - 1.f);
		glEnable(GL_DEPTH_TEST);
	}

	//RenderGrid();
	DoSelectedEntityHighlightPass();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (m_state.simulate_mode_active) { // Render fullscreen image
		Renderer::GetShaderLibrary().GetQuadShader().ActivateProgram();
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_res.p_scene_display_texture->GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR);
		Renderer::DrawQuad();
	}
}




void EditorLayer::RenderGeneralSettingsMenu() {
	if (Window::Get().input.IsKeyDown(Key::LeftControl) && Window::Get().input.IsKeyPressed('j')) {
		m_state.general_settings.editor_window_settings.display_settings_window = !m_state.general_settings.editor_window_settings.display_settings_window;
	}

	if (!m_state.general_settings.editor_window_settings.display_settings_window)
		return;

	if (ImGui::Begin("Settings")) {
		ImGui::SeparatorText("Editor camera");
		ImGui::Text("Exposure");
		ImGui::SliderFloat("##exposure", &mp_editor_camera->GetComponent<CameraComponent>()->exposure, 0.f, 10.f);

		ImGui::SeparatorText("Debug rendering");
		ImGui::Checkbox("Render physx debug", &m_state.general_settings.debug_render_settings.render_physx_debug);
		ImGui::Checkbox("Wireframe mode", &m_state.general_settings.debug_render_settings.render_wireframe);
		ImGui::Checkbox("Render meshes", &m_state.general_settings.debug_render_settings.render_meshes);
		static int voxel_mip = 0;
		if (ImGui::InputInt("Voxel mip", &voxel_mip) && voxel_mip >= 0 && voxel_mip <= 6) {
			m_state.general_settings.debug_render_settings.voxel_mip = voxel_mip;
		}

		const char* faces[8] = { "NONE", "BASE", "POS_X", "POS_Y", "POS_Z", "NEG_X", "NEG_Y", "NEG_Z" };
		static int current_face = 0;

		if (ImGui::BeginCombo("Voxel face", faces[current_face])) {
			for (int i = 0; i < 8; i++) {
				bool selected = current_face == i;

				if (ImGui::Selectable(faces[i], &selected)) {
					current_face = i;
					m_state.general_settings.debug_render_settings.voxel_render_face = static_cast<VoxelRenderFace>(1 << i);
				}
			}
			ImGui::EndCombo();
		}

		ImGui::SeparatorText("Selection");
		ImGui::Checkbox("Select physics objects", &m_state.general_settings.selection_settings.select_physics_objects);
		ImGui::Checkbox("Select mesh objects", &m_state.general_settings.selection_settings.select_mesh_objects);
		ImGui::Checkbox("Select lights", &m_state.general_settings.selection_settings.select_light_objects);
		ImGui::Checkbox("Select all", &m_state.general_settings.selection_settings.select_all);
		ImGui::Checkbox("Select only parents", &m_state.general_settings.selection_settings.select_only_parents);
	}
	ImGui::End();
}

void EditorLayer::RenderToolbar() {
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
	ImGui::SetNextWindowSize(ImVec2(static_cast<float>(Window::GetWidth()), static_cast<float>(m_res.toolbar_height)));

	if (ImGui::Begin("##Toolbar", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNavInputs)) {
		if (ImGui::Button("Files")) {
			ImGui::OpenPopup("FilePopup");
		}
		static std::vector<std::string> file_options{ "New project", "Open project", "Save project", "Build game"};
		static int selected_component = 0;

		if (ImGui::BeginPopup("FilePopup"))
		{
			ImGui::PushID(1);
			ImGui::SeparatorText("Files");
			ImGui::PopID();
			for (size_t i = 0; i < file_options.size(); i++) {
				if (ImGui::Selectable(file_options[i].c_str()))
					selected_component = static_cast<int>(i) + 1;
			}
			ImGui::EndPopup();
		}

		switch (selected_component) {
		case 1:
			RenderProjectGenerator(selected_component);
			// Allow selected_component to keep its value so this can continue rendering until another option is picked
			break;
		case 2: {
			OpenLoadProjectMenu();
			selected_component = 0;
			break;
		}
		case 3: {
			SaveProject();
			selected_component = 0;
			break;
		}
		case 4: {
#ifndef _DEBUG
			m_state.show_build_menu = true;
#else
			ORNG_CORE_CRITICAL("Building games is not allowed in editor debug mode, use a release build instead.");
#endif
			selected_component = 0;
			break;
		}
		default:
			break;
		}


		ImGui::SameLine();
		if (m_state.simulate_mode_active && ImGui::Button(ICON_FA_CIRCLE))
			EndPlayScene();
		else if (!m_state.simulate_mode_active && ImGui::Button(ICON_FA_PLAY))
			BeginPlayScene();

		ImGui::SameLine();
		if (m_state.simulate_mode_active &&
			((!m_state.simulate_mode_paused && ImGui::Button(ICON_FA_PAUSE)) ||
			(m_state.simulate_mode_paused && ImGui::Button(ICON_FA_ANGLES_UP)
			))) {
			m_state.simulate_mode_paused = !m_state.simulate_mode_paused;
		}


		ImGui::SameLine();

		if (ImGui::Button("Reload shaders")) {
			Renderer::GetShaderLibrary().ReloadShaders();
		}
		ImGui::SameLine();


		if (!m_state.simulate_mode_active && ExtraUI::SwitchButton("VR", m_state.use_vr_in_simulation, m_res.blue_col)) {
			SetVrMode(!m_state.use_vr_in_simulation);
		}
		ImGui::SameLine();

		// Transform gizmos
		ImGui::Dummy({ 100, 0 });
		ImGui::SameLine();
		if (ExtraUI::SwitchButton(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT, m_state.current_gizmo_operation == ImGuizmo::TRANSLATE, m_res.blue_col))
			m_state.current_gizmo_operation = ImGuizmo::TRANSLATE;
		ImGui::SameLine();

		if (ExtraUI::SwitchButton(ICON_FA_COMPRESS, m_state.current_gizmo_operation == ImGuizmo::SCALE, m_res.blue_col))
			m_state.current_gizmo_operation = ImGuizmo::SCALE;
		ImGui::SameLine();

		if (ExtraUI::SwitchButton(ICON_FA_ROTATE_RIGHT, m_state.current_gizmo_operation == ImGuizmo::ROTATE, m_res.blue_col))
			m_state.current_gizmo_operation = ImGuizmo::ROTATE;
		ImGui::SameLine();

		if (!ImGui::GetIO().WantCaptureMouse) {
			if (Window::Get().input.IsKeyPressed(GLFW_KEY_1))
				m_state.current_gizmo_operation = ImGuizmo::TRANSLATE;
			else if (Window::Get().input.IsKeyPressed(GLFW_KEY_2))
				m_state.current_gizmo_operation = ImGuizmo::SCALE;
			else if (Window::Get().input.IsKeyPressed(GLFW_KEY_3))
				m_state.current_gizmo_operation = ImGuizmo::ROTATE;
		}


		if (ImGui::Button("Transform mode")) {
			ImGui::OpenPopup("##transform mode");
		}

		if (ImGui::BeginPopup("##transform mode")) {
			if (ImGui::Selectable("Entities", m_state.selection_mode == SelectionMode::ENTITY))
				m_state.selection_mode = SelectionMode::ENTITY;

			ImGui::EndPopup();
		}

		if (!ImGui::GetIO().WantCaptureMouse && Window::Get().input.IsKeyPressed(GLFW_KEY_4)) {
			m_state.selection_mode = SelectionMode::ENTITY;
		}

		// Additional windows
		ImGui::SameLine();
		ImGui::Dummy({ 100, 0 });
		ImGui::SameLine();
		if (ExtraUI::SwitchButton(ICON_FA_DIAGRAM_PROJECT, m_state.general_settings.editor_window_settings.display_joint_maker, m_res.blue_col)) {
			m_state.general_settings.editor_window_settings.display_joint_maker = !m_state.general_settings.editor_window_settings.display_joint_maker;
		}
		ExtraUI::TooltipOnHover("Joint maker");
		ImGui::SameLine();

		if (ExtraUI::SwitchButton(ICON_FA_P, m_state.general_settings.debug_render_settings.render_physx_debug, m_res.blue_col)) {
			m_state.general_settings.debug_render_settings.render_physx_debug = !m_state.general_settings.debug_render_settings.render_physx_debug;
		}
		ExtraUI::TooltipOnHover("Debug physx rendering");
		ImGui::SameLine();

		/*ImGui::Text("Gizmo rendering");
		if (ImGui::RadioButton("World", current_mode == ImGuizmo::WORLD))
			current_mode = ImGuizmo::WORLD;
		ImGui::SameLine();
		if (ImGui::RadioButton("Local", current_mode == ImGuizmo::LOCAL))
			current_mode = ImGuizmo::LOCAL;*/

		std::string sep_text = "Project: " + m_state.current_project_directory.substr(m_state.current_project_directory.find_last_of("\\") + 1);
		ImGui::SeparatorText(sep_text.c_str());
	}
	ImGui::End();

}

void EditorLayer::OpenLoadProjectMenu() {
	wchar_t valid_extensions[MAX_PATH] = L"Project Files: *.oproj\0*.oproj\0";
	std::function<void(std::string)> success_callback = [this](std::string filepath) {
		MakeProjectActive(filepath.substr(0, filepath.find_last_of('\\')));
	};
	ExtraUI::ShowFileExplorer(valid_extensions, success_callback);
}

void EditorLayer::SaveProject() {
	std::string uuid_filepath{ "./res/scripts/includes/uuids.h" };
	AssetManager::GetSerializer().SerializeAssets();
	auto* p_scene_asset = AssetManager::GetAsset<SceneAsset>(SCENE->m_asset_uuid());
	std::string write_path = p_scene_asset ? p_scene_asset->filepath : "res/scene_temp.oscene";
	std::string serialized;

	if (!p_scene_asset) {
		SCENE->m_asset_uuid = UUID<uint64_t>{};
		p_scene_asset = AssetManager::AddAsset(new SceneAsset{"res/scene_temp.oscene", SCENE->m_asset_uuid()});
	}

	SceneSerializer::SerializeScene(*SCENE, serialized, true);
	WriteTextFile(write_path, serialized);

	// Update scene asset contents
	p_scene_asset->node = YAML::Load(serialized);

	// Update UUID file
	SceneSerializer::SerializeSceneUUIDs(AssetManager::GetView<SceneAsset>(), uuid_filepath);

	SerializeProjectToFile(m_state.current_project_directory + "/project.oproj");
}

void EditorLayer::GenerateGameRuntimeSettings(const std::string &output_path, const RuntimeSettings& settings) {
	std::vector<std::byte> bin;
	bitsery::Serializer<bitsery::OutputBufferAdapter<std::vector<std::byte>>> s{bin};
	s.object(settings);
	WriteBinaryFile(output_path, bin.data(), bin.size());
}

void EditorLayer::BuildGameFromActiveProject() {
	// Delete old build
	TryFileDelete("./build");
	Create_Directory("./build/res");

	GenerateGameRuntimeSettings(m_state.current_project_directory + "/build/runtime.orts", m_state.build_runtime_settings);

	for (auto& file : std::filesystem::recursive_directory_iterator{ m_state.current_project_directory + "/res" }) {
		auto path_str = file.path().generic_string();
		if (path_str.find(".vs") != std::string::npos)
			continue;

		std::string rel_path = path_str.substr(path_str.find("/res"));
		if (file.is_directory()) {
			Create_Directory("./build" + rel_path);
		}
		else {
			FileCopy(path_str, "./build" + rel_path);
		}
	}

	FileCopy(GetApplicationExecutableDirectory() + "/res/shaders", "build/res/shaders", true);
	FileCopy(GetApplicationExecutableDirectory() + "/../ORNG-Runtime/ORNG_RUNTIME.exe", "build/ORNG_RUNTIME.exe");

	std::vector<std::string> dlls = {
		"fmod.dll", "PhysX_64.dll", "PhysXCommon_64.dll", "PhysXCooking_64.dll", "PhysXFoundation_64.dll", "glew-shared.dll"
	};

	for (const auto& path : dlls) {
		FileCopy(GetApplicationExecutableDirectory() + "/../ORNG-Runtime/" + path, "build/" + path);
	}
}

void EditorLayer::RenderBuildMenu() {
	//ImVec2 window_size{600, 600};
	//ImVec2 window_pos{(static_cast<float>(Window::GetWidth()) - window_size.x) / 2.f, (static_cast<float>(Window::GetWidth()) - window_size.x) / 2.f};

	if (ImGui::Begin("Build settings")) {
		ImGui::Checkbox("VR runtime", &m_state.build_runtime_settings.use_vr);

		auto* p_current_start_scene = AssetManager::GetAsset<SceneAsset>(m_state.build_runtime_settings.start_scene_uuid);
		std::string start_scene_name = p_current_start_scene ? p_current_start_scene->node["Scene"].as<std::string>() : "NONE";

		ImGui::Text("%s", std::format("Start scene: {}", start_scene_name).c_str()); ImGui::SameLine(); ImGui::Button("Drop scene asset here");
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("SCENE ASSET")) {
				if (p_payload->DataSize == sizeof(SceneAsset*)) {
					m_state.build_runtime_settings.start_scene_uuid = (*static_cast<SceneAsset**>(p_payload->Data))->uuid();
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	if (ImGui::Button("Build")) {
		m_state.show_build_menu = false;
		BuildGameFromActiveProject();
	}

	if (ImGui::Button("Exit")) m_state.show_build_menu = false;
}

void EditorLayer::RenderProjectGenerator(int& selected_component_from_popup) {
	ImGui::SetNextWindowSize(ImVec2(500, 200));
	ImGui::SetNextWindowPos(ImVec2(static_cast<float>(Window::GetWidth() / 2 - 250), static_cast<float>(Window::GetHeight() / 2 - 100)));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10);
	static std::string project_dir;

	if (ImGui::Begin("##project gen", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
		if (ImGui::IsMouseDoubleClicked(1)) // close window
			selected_component_from_popup = 0;

		ImGui::PushFont(m_res.p_xl_font);
		ImGui::SeparatorText("Generate project");
		ImGui::PopFont();
		ImGui::Text("Path");
		ImGui::InputText("#pdir", &project_dir);

		static std::string err_msg = "";
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
		ImGui::TextWrapped("%s", err_msg.c_str());
		ImGui::PopStyleColor();

		if (ImGui::Button("Generate")) {
			if (FileExists(project_dir) && GenerateProject(project_dir, true)) {
				MakeProjectActive(project_dir);
				selected_component_from_popup = 0;
			}
			else {
				err_msg = Log::GetLastLog();
			}
		}
	}
	ImGui::End();
	ImGui::PopStyleVar(); // window rounding
}

// TEMPORARY - while stuff is actively changing here just refresh it automatically so I don't have to manually delete it each time
static void RefreshScriptIncludes() {
	FileCopy(ORNG_CORE_MAIN_DIR "/headers/scripting/ScriptAPI.h", "./res/scripts/includes/ScriptAPI.h");
	FileCopy(ORNG_CORE_MAIN_DIR "/headers/scripting/ScriptShared.h", "./res/scripts/includes/ScriptShared.h");
}

void EditorLayer::SetActiveScene(SceneAsset& scene) {
	m_event_stack.Clear();
	mp_editor_camera = nullptr;
	std::string before; SceneSerializer::SerializeScene(*SCENE, before, true);
	uint64_t before_uuid = SCENE->m_asset_uuid();

	SCENE->UnloadScene();

	AddDefaultSceneSystems();
	SCENE->LoadScene();

	mp_editor_camera = std::make_unique<SceneEntity>(&*SCENE, SCENE->m_registry.create(), &SCENE->m_registry, SCENE->m_static_uuid());
	mp_editor_camera->AddComponent<TransformComponent>();

	if (!SceneSerializer::DeserializeScene(*SCENE, "", false, &scene.node)) {
		ORNG_CORE_ERROR("Failed to set active scene, reverting back to previous scene.");
		SCENE->UnloadScene();
		SCENE->m_asset_uuid = UUID<uint64_t>{before_uuid};
		AddDefaultSceneSystems();
		SCENE->LoadScene();
		SceneSerializer::DeserializeScene(*SCENE, before, false);
	}

	mp_editor_camera->AddComponent<CameraComponent>()->MakeActive();

	InitRenderGraph(m_render_graph, false);
	if (m_state.use_vr_in_simulation) InitRenderGraph(*m_state.p_vr_render_graph, true);
}

bool EditorLayer::GenerateProject(const std::string& project_name, bool abs_path) {
	if (std::filesystem::exists(m_state.executable_directory + "/projects/" + project_name)) {
		ORNG_CORE_ERROR("Project with name '{0}' already exists", project_name);
		return false;
	}

	if (!std::filesystem::exists(m_state.executable_directory + "/projects"))
		std::filesystem::create_directory(m_state.executable_directory + "/projects");

	std::string project_path = abs_path ? project_name : m_state.executable_directory + "/projects/" + project_name;
	Create_Directory(project_path);

	std::filesystem::create_directory(project_path + "/res");
	std::filesystem::create_directory(project_path + "/res/meshes");
	std::filesystem::create_directory(project_path + "/res/textures");
	std::filesystem::create_directory(project_path + "/res/scripts");
	std::filesystem::create_directory(project_path + "/res/scenes");
	std::filesystem::create_directory(project_path + "/res/scripts/includes");
	std::filesystem::create_directories(project_path + "/res/scripts/bin/release");
	std::filesystem::create_directories(project_path + "/res/scripts/bin/debug");
	std::filesystem::create_directory(project_path + "/res/shaders");
	std::filesystem::create_directory(project_path + "/res/audio");
	std::filesystem::create_directory(project_path + "/res/prefabs");
	std::filesystem::create_directory(project_path + "/res/materials");
	std::filesystem::create_directory(project_path + "/res/physx-materials");

	// Create base scene for project to use
	std::ofstream s{ project_path + "/res/scenes/scene.oscene" };
	s << ORNG_BASE_SCENE_YAML;
	s.close();

	s = std::ofstream{ project_path + "/project.oproj"};
	s << ORNG_BASE_EDITOR_PROJECT_YAML;
	s.close();

	return true;
}


bool EditorLayer::ValidateProjectDir([[maybe_unused]] const std::string& dir_path) {
	// For now no extra validation is done
	return true;
}


void EditorLayer::AddDefaultSceneSystems() {
	SCENE->AddSystem(new CameraSystem{ SCENE }, 0);
	SCENE->AddSystem(new EnvMapSystem{ SCENE }, 1000);
	SCENE->AddSystem(new AudioSystem{ SCENE }, 2000);
	SCENE->AddSystem(new PointlightSystem{ SCENE }, 3000);
	SCENE->AddSystem(new SpotlightSystem{ SCENE }, 4000);
	SCENE->AddSystem(new ParticleSystem{ SCENE }, 5000);
	SCENE->AddSystem(new PhysicsSystem{ SCENE }, 6000);
	SCENE->AddSystem(new TransformHierarchySystem{ SCENE }, 7000);
	SCENE->AddSystem(new ScriptSystem{ SCENE }, 8000);
	SCENE->AddSystem(new SceneUBOSystem{ SCENE }, 9000);
	SCENE->AddSystem(new MeshInstancingSystem{ SCENE }, 10000);
}


bool EditorLayer::MakeProjectActive(const std::string& folder_path) {
	ORNG_CORE_INFO("Attempting to make project '{0}' active", folder_path);

	if (ValidateProjectDir(folder_path)) {
		if (m_state.simulate_mode_active)
			EndPlayScene();

		m_state.current_project_directory = std::filesystem::absolute(folder_path).string();
		std::filesystem::current_path(folder_path);

		// Update resources
		FileCopy(GetApplicationExecutableDirectory() + "/res/", m_state.current_project_directory + "/res/", true);
		RefreshScriptIncludes();

		// Deselect material and texture that's about to be deleted
		m_asset_manager_window.SelectMaterial(nullptr);
		m_asset_manager_window.SelectTexture(nullptr);

		m_state.selected_entity_ids.clear();

		glm::vec3 cam_pos = mp_editor_camera ? mp_editor_camera->GetComponent<TransformComponent>()->GetAbsPosition() : glm::vec3{0, 0, 0};
		mp_editor_camera = nullptr; // Delete explicitly here to properly remove it from the scene before unloading

		if (SCENE->m_is_loaded)
			SCENE->UnloadScene();

		AssetManager::ClearAll();
		AssetManager::GetSerializer().LoadAssetsFromProjectPath(m_state.current_project_directory);

		mp_editor_camera = std::make_unique<SceneEntity>(&*SCENE, SCENE->m_registry.create(), &SCENE->m_registry, SCENE->m_static_uuid());
		DeserializeProjectFromFile(m_state.current_project_directory + "/project.oproj");
		mp_editor_camera->AddComponent<TransformComponent>()->SetAbsolutePosition(cam_pos);
		mp_editor_camera->AddComponent<CameraComponent>()->MakeActive();

		// Reinitialize with new scene system resources
		m_render_graph.Reset();
		// This render graph is for the editor, not the simulation mode, so keep VR disabled
		InitRenderGraph(m_render_graph, false);

		EventManager::DispatchEvent(EditorEvent(EditorEventType::POST_SCENE_LOAD));
	}
	else {
		ORNG_CORE_ERROR("Project folder path invalid");
		return false;
	}
	return true;
}

void EditorLayer::SerializeProjectToFile(const std::string &output_path) {
	YAML::Emitter emitter;

	auto* p_cam_transform = mp_editor_camera->GetComponent<TransformComponent>();
	emitter << YAML::BeginMap;
	emitter << YAML::Key << "CamPos" << YAML::Value << p_cam_transform->GetAbsPosition();
	emitter << YAML::Key << "CamFwd" << YAML::Value << p_cam_transform->forward;
	emitter << YAML::Key << "VR_Enabled" << YAML::Value << m_state.use_vr_in_simulation;
	emitter << YAML::Key << "ActiveSceneUUID" << YAML::Value << SCENE->m_asset_uuid();
	emitter << YAML::EndMap;

	WriteTextFile(output_path, std::string{emitter.c_str()});
}

void EditorLayer::DeserializeProjectFromFile(const std::string &input_path) {
	std::string file_content = ReadTextFile(input_path);
	if (file_content.empty()) return;

	YAML::Node node = YAML::Load(file_content);
	AddDefaultSceneSystems();
	SCENE->LoadScene();

	auto uuid = node["ActiveSceneUUID"].as<uint64_t>();
	auto* p_scene_asset = AssetManager::GetAsset<SceneAsset>(uuid);

	if (!p_scene_asset) {
		ORNG_CORE_ERROR("Project deserialization error: active scene UUID '{}' cannot be found", uuid);
	} else {
		SceneSerializer::DeserializeScene(*SCENE, "", false, &p_scene_asset->node);
	}

	auto* p_cam_transform = mp_editor_camera->GetComponent<TransformComponent>();
	const auto pos = node["CamPos"].as<glm::vec3>();
	const auto fwd = node["CamFwd"].as<glm::vec3>();
	p_cam_transform->SetAbsolutePosition(pos);
	p_cam_transform->LookAt(pos + fwd);

	SetVrMode(node["VR_Enabled"].as<bool>());
}

void EditorLayer::RenderCreationWidget(SceneEntity* p_entity, bool trigger) {
	std::array names = { "Pointlight", "Spotlight", "Mesh", "Camera", "Physics", "Script", "Audio", "Vehicle", "Particle emitter", "Billboard", "Particle buffer", "Character controller", "Joint"};

	if (trigger)
		ImGui::OpenPopup("my_select_popup");

	int selected_component = -1;
	if (ImGui::BeginPopup("my_select_popup")) {
		ImGui::SeparatorText(p_entity ? "Add component" : "Create entity");
		for (size_t i = 0; i < names.size(); i++)
			if (ImGui::Selectable(names[i]))
				selected_component = static_cast<int>(i);

		ImGui::EndPopup();
	}

	if (selected_component == -1)
		return;

	auto* entity = p_entity ? p_entity : &CreateEntityTracked("New entity");
	SelectEntity(entity->GetUUID());

	switch (selected_component) {
	case 0:
		entity->AddComponent<PointLightComponent>();
		break;
	case 1:
		entity->AddComponent<SpotLightComponent>();
		break;
	case 2:
		entity->AddComponent<MeshComponent>();
		break;
	case 3:
		entity->AddComponent<CameraComponent>();
		break;
	case 4:
		entity->AddComponent<PhysicsComponent>();
		break;
	case 5:
		entity->AddComponent<ScriptComponent>();
		break;
	case 6:
		entity->AddComponent<AudioComponent>();
		break;
	case 8:
		entity->AddComponent<ParticleEmitterComponent>();
		break;
	case 9:
		entity->AddComponent<BillboardComponent>();
		break;
	case 10:
		entity->AddComponent<ParticleBufferComponent>();
		break;
	case 11:
		entity->AddComponent<CharacterControllerComponent>();
		break;
	case 12:
		entity->AddComponent<JointComponent>();
		break;
	default:
		BREAKPOINT;
	}
}



glm::vec2 EditorLayer::ConvertFullscreenMouseToDisplayMouse(glm::vec2 mouse_coords) {
	// Transform mouse coordinates to full window space for the proper texture coordinates
	mouse_coords.x *= (static_cast<float>(Window::GetWidth()) / static_cast<float>(Window::GetWidth() - (RIGHT_WINDOW_WIDTH)));
	mouse_coords.y -= m_res.toolbar_height;
	mouse_coords.y *= static_cast<float>(Window::GetHeight()) / static_cast<float>(Window::GetHeight() - BOTTOM_WINDOW_HEIGHT - m_res.toolbar_height);
	return mouse_coords;
}


void EditorLayer::DoPickingPass() {
	m_res.picking_fb.Bind();
	m_res.picking_shader.ActivateProgram();

	GL_StateManager::ClearDepthBits();
	GL_StateManager::ClearBitsUnsignedInt(UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX);

	// Mesh picking
	auto view = SCENE->m_registry.view<MeshComponent>();
	for (auto [entity, mesh] : view.each()) {
		//Split uint64 into two uint32's for texture storage
		uint64_t full_id = mesh.GetEntityUUID();
		glm::uvec3 id_vec{ static_cast<uint32_t>(full_id >> 32), static_cast<uint32_t>(full_id), UINT_MAX };

		m_res.picking_shader.SetUniform("comp_id", id_vec);
		m_res.picking_shader.SetUniform("transform", mesh.GetEntity()->GetComponent<TransformComponent>()->GetMatrix());

		Renderer::DrawMeshInstanced(mesh.GetMeshData(), 1);
	}

	glm::vec2 mouse_coords = glm::min(
		glm::max(glm::vec2(Window::Get().input.GetMousePos()), glm::vec2(1, 1)),
		glm::vec2(Window::GetWidth() - 1, Window::GetHeight() - 1)
	);

	if (!m_state.fullscreen_scene_display) {
		mouse_coords = ConvertFullscreenMouseToDisplayMouse(mouse_coords);
	}

	uint32_t* pixels = new uint32_t[3];
	glReadPixels(static_cast<int>(mouse_coords.x), Window::GetHeight() - static_cast<int>(mouse_coords.y), 1, 1, GL_RGB_INTEGER, GL_UNSIGNED_INT, pixels);
	uint64_t current_entity_id = (static_cast<uint64_t>(pixels[0]) << 32) | pixels[1];

	if (!Window::Get().input.IsKeyDown(GLFW_KEY_LEFT_CONTROL))
		m_state.selected_entity_ids.clear();

	if (pixels[2] != UINT_MAX) { // Joint selected
		auto* p_ent = SCENE->GetEntity(current_entity_id);

		if (p_ent) {
			SelectEntity(current_entity_id);
		}
	}
	else { // Mesh selected
		SelectEntity(current_entity_id);
	}

	delete[] pixels;

}




void EditorLayer::DoSelectedEntityHighlightPass() {
	m_res.editor_pass_fb.Bind();
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glClearStencil(0);
	glClear(GL_STENCIL_BUFFER_BIT);

	glStencilFunc(GL_ALWAYS, 1, 0xFF);

	glDisable(GL_DEPTH_TEST);
	m_res.highlight_shader.ActivateProgram();
	m_res.highlight_shader.SetUniform("u_colour", glm::vec4(0.0, 1, 0, 0));

	for (auto id : m_state.selected_entity_ids) {
		auto* current_entity = SCENE->GetEntity(id);

		if (!current_entity || !current_entity->HasComponent<MeshComponent>())
			continue;

		MeshComponent* meshc = current_entity->GetComponent<MeshComponent>();

		m_res.highlight_shader.SetUniform("u_scale", 1.f);
		m_res.highlight_shader.SetUniform("transform", meshc->GetEntity()->GetComponent<TransformComponent>()->GetMatrix());
		Renderer::DrawMeshInstanced(meshc->GetMeshData(), 1);
	}

	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);


	for (auto id : m_state.selected_entity_ids) {
		auto* current_entity = SCENE->GetEntity(id);
		m_res.highlight_shader.SetUniform("u_colour", glm::vec4(1.0, 0.2, 0, 1));

		if (!current_entity || !current_entity->HasComponent<MeshComponent>())
			continue;

		MeshComponent* meshc = current_entity->GetComponent<MeshComponent>();

		m_res.highlight_shader.SetUniform("u_scale", 1.025f);

		m_res.highlight_shader.SetUniform("transform", meshc->GetEntity()->GetComponent<TransformComponent>()->GetMatrix());
		Renderer::DrawMeshInstanced(meshc->GetMeshData(), 1);
	}

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);

}


void EditorLayer::RenderGrid() {
	m_res.grid_mesh->CheckBoundary(mp_editor_camera->GetComponent<TransformComponent>()->GetPosition());
	GL_StateManager::BindSSBO(m_res.grid_mesh->m_transform_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::TRANSFORMS);

	m_res.grid_shader.ActivateProgram();
	Renderer::DrawVAO_ArraysInstanced(GL_LINES, m_res.grid_mesh->m_vao, static_cast<int>(ceil(m_res.grid_mesh->grid_width / m_res.grid_mesh->grid_step)) * 2);
}


void EditorLayer::UpdateLuaEntityArray() {
	std::string reset_script = R"(
		entity_array = 1
		entity_array = {}
	)";

	for (size_t i = 0; i < SCENE->m_entities.size(); i++) {
		auto* p_ent = SCENE->m_entities[i];
		auto* p_transform = p_ent->GetComponent<TransformComponent>();
		auto* p_relation_comp = p_ent->GetComponent<RelationshipComponent>();

		auto pos = p_transform->GetPosition();
		auto scale = p_transform->GetScale();
		auto rot = p_transform->GetOrientation();

		std::string entity_push_script = std::format("entity_array[{0}] = entity.new(\"{1}\", {2}, vec3.new({3}, {4}, {5}),  vec3.new({6}, {7}, {8}), vec3.new({9}, {10}, {11}), {12})",
			i+1, p_ent->name, static_cast<unsigned>(p_ent->GetEnttHandle()), pos.x, pos.y, pos.z, scale.x, scale.y, scale.z, rot.x, rot.y, rot.z,
			static_cast<unsigned>(p_relation_comp->parent));

		m_lua_cli.GetLua().script(entity_push_script);
	}
}


void EditorLayer::InitLua() {
	m_lua_cli.Init();
	m_lua_cli.input_callbacks.push_back([this] {
		UpdateLuaEntityArray();
		});

	auto& lua = m_lua_cli.GetLua();

	lua.set_function("ORNG_select_entity", [this](unsigned handle) {
		auto* p_ent = SCENE->GetEntity(static_cast<entt::entity>(handle));
		if (p_ent)
			m_state.selected_entity_ids.push_back(p_ent->GetUUID());
		});

	lua.set_function("get_entity", [this](unsigned handle) -> LuaEntity {
		auto* p_ent = SCENE->GetEntity(static_cast<entt::entity>(handle));
		if (!p_ent)
			return LuaEntity{ "NULL", static_cast<unsigned>(entt::null), {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, static_cast<unsigned>(entt::null) };

		auto* p_transform = p_ent->GetComponent<TransformComponent>();
		auto* p_relation_comp = p_ent->GetComponent<RelationshipComponent>();

		return LuaEntity{ p_ent->name, static_cast<unsigned>(p_ent->GetEnttHandle()), p_transform->GetPosition(),
			p_transform->GetScale(), p_transform->GetOrientation(), static_cast<unsigned>(p_relation_comp->parent)};
		});

	lua.set_function("clear", [this] {
		m_logger_ui.ClearLogs();
	});

	std::string util_script = R"(
		entity_array = {}
		pos = 0;

		function evaluate(expression, entity)
			local chunk, err_message = load("return " .. expression, "c", "t", {entity = entity, get_entity = get_entity})

			if not chunk then
				error("Error in expression: " .. err_message)
			end

			return chunk()
		end

		function select(expression)
			num_selected = 0;

			for i, entity in ipairs(entity_array) do
				if (evaluate(expression, entity)) then
					ORNG_select_entity(entity.entt_handle)
					num_selected = num_selected + 1
				end
			end
			print(tostring(num_selected) .. " entities selected")
		end

)";

	m_lua_cli.GetLua().script(util_script);

#define TRANSFORM_LUA_SKELETON(x) 	EditorEntityEvent e{ TRANSFORM_UPDATE, m_state.selected_entity_ids }; \
	for (auto id : m_state.selected_entity_ids) { \
		auto* p_ent = SCENE->GetEntity(id); \
\
		auto* p_transform = p_ent->GetComponent<TransformComponent>(); \
		e.serialized_entities_before.push_back(SceneSerializer::SerializeEntityIntoString(*p_ent)); \
		x; \
	} \
\
	m_event_stack.PushEvent(e); \


	std::function<void(glm::vec3)> p_translate_func = [this](glm::vec3 v) {
		TRANSFORM_LUA_SKELETON(p_transform->SetPosition(p_transform->GetPosition() + v))
		};

	std::function<void(glm::vec3)> p_scale_func = [this](glm::vec3 v) {
		TRANSFORM_LUA_SKELETON(p_transform->SetScale(p_transform->GetScale() * v))
		};

	std::function<void(glm::vec3, float)> p_rot_func = [this](glm::vec3 axis, float angle_degrees) {
		TRANSFORM_LUA_SKELETON(p_transform->SetOrientation(glm::degrees(glm::eulerAngles(glm::angleAxis(glm::radians(angle_degrees), axis) * glm::quat(glm::radians(p_transform->GetOrientation()))))))
		};

	std::function<void(glm::vec3)> p_move_to_func = [this](glm::vec3 pos) {
		TRANSFORM_LUA_SKELETON(p_transform->SetAbsolutePosition(pos))
		};

	std::function<void(glm::vec3)> p_cam_move_to_func = [this](glm::vec3 pos) {
		mp_editor_camera->GetComponent<TransformComponent>()->SetPosition(pos);
		};

	std::function<void()> p_match_func = [this]() {
		auto* p_cam_transform = mp_editor_camera->GetComponent<TransformComponent>();
		TRANSFORM_LUA_SKELETON(
			p_transform->SetAbsolutePosition(p_cam_transform->GetPosition());
			p_transform->LookAt(p_cam_transform->GetPosition() + p_cam_transform->forward);
		)
		};


	std::function<void(glm::vec3, float, glm::vec3)> p_rot_about_point_func = [this](glm::vec3 axis, float angle_degrees, glm::vec3 pivot) {
		TRANSFORM_LUA_SKELETON(
			glm::quat q = glm::angleAxis(glm::radians(angle_degrees), axis);
			glm::vec3 offset_pos = p_transform->GetAbsPosition() - pivot;
			p_transform->SetAbsolutePosition(q * offset_pos + pivot);
			p_transform->SetOrientation(glm::degrees(glm::eulerAngles(glm::angleAxis(glm::radians(angle_degrees), axis) * glm::quat(glm::radians(p_transform->GetOrientation())))))
			)
		};

#undef TRANSFORM_LUA_SKELETON

	m_lua_cli.GetLua().set_function("translate", p_translate_func);
	m_lua_cli.GetLua().set_function("scale", p_scale_func);
	m_lua_cli.GetLua().set_function("rotate", p_rot_func);
	m_lua_cli.GetLua().set_function("rotate_about", p_rot_about_point_func);
	m_lua_cli.GetLua().set_function("moveto", p_move_to_func);
	m_lua_cli.GetLua().set_function("move_me", p_cam_move_to_func);
	m_lua_cli.GetLua().set_function("match", p_match_func);

}

void EditorLayer::RenderSkyboxEditor() {
	if (ExtraUI::H1TreeNode("Skybox")) {
		wchar_t valid_extensions[MAX_PATH] = L"Texture Files: *.png;*.jpg;*.jpeg;*.hdr\0*.png;*.jpg;*.jpeg;*.hdr\0";

		static bool using_env_maps = true;
		static int resolution = 4096;

		auto& skybox = SCENE->GetSystem<EnvMapSystem>().skybox;
		using_env_maps = skybox.using_env_map;
		resolution = skybox.m_resolution;

		std::function<void(std::string)> file_explorer_callback = [this](std::string filepath) {
			// Check if texture is an asset or not, if not, add it
			std::string new_filepath = "./res/textures/" + filepath.substr(filepath.find_last_of("\\") + 1);
			if (!std::filesystem::exists(new_filepath)) {
				FileCopy(filepath, new_filepath);
			}

			auto& env_map_system = SCENE->GetSystem<EnvMapSystem>();
			env_map_system.skybox.using_env_map = using_env_maps;
			env_map_system.LoadSkyboxFromHDRFile(new_filepath, resolution);
			};

		if (ImGui::Button("Load skybox texture")) {
			ExtraUI::ShowFileExplorer(valid_extensions, file_explorer_callback);
		}
		ImGui::SameLine();
		ImGui::SmallButton("?");

		if (ImGui::BeginItemTooltip()) {
			ImGui::Text("Converts an equirectangular image into a cubemap for use in a skybox. For best results, use HDRI's!");
			ImGui::EndTooltip();
		}

		ImGui::InputInt("Resolution", &skybox.m_resolution);

		if (ImGui::Checkbox("Gen IBL textures", &using_env_maps) || ImGui::Button("Reload")) {
			skybox.using_env_map = using_env_maps;
			SCENE->GetSystem<EnvMapSystem>().LoadSkyboxFromHDRFile(skybox.GetSrcFilepath(), resolution);
		}
	}
}


void EditorLayer::PushEntityIntoGraph(SceneEntity* p_entity, std::vector<EditorLayer::EntityNodeEntry>& output, unsigned depth) {
	output.push_back(EditorLayer::EntityNodeEntry{ .p_entity = p_entity, .depth = depth });
	if (VectorContains(m_state.open_tree_nodes_entities, p_entity->GetUUID())) {
		p_entity->ForEachLevelOneChild([&](entt::entity child) { PushEntityIntoGraph(SCENE->GetEntity(child), output, depth + 1); });
	}
}

void EditorLayer::GetEntityGraph(std::vector<EditorLayer::EntityNodeEntry>& output) {
	for (auto entt_handle : SCENE->m_root_entities) {
		PushEntityIntoGraph(SCENE->GetEntity(entt_handle), output, 0);
	}
}

EntityNodeData EditorLayer::RenderEntityNode(SceneEntity* p_entity, unsigned int layer, bool node_selection_active, const Box2D& selection_box) {
	EntityNodeData ret{ EntityNodeEvent::E_NONE, {0, 0}, {0, 0} };

	static std::string padding_str;
	for (unsigned i = 0; i < layer; i++) {
		padding_str += " |--|";
	}
	ImGui::Text("%s", padding_str.c_str());
	padding_str.clear();
	ImGui::SameLine();
	// Setup display name with icons
	static std::string formatted_name; // Static to stop a new string being made every single call, only needed for c_str anyway
	formatted_name.clear();

	formatted_name = p_entity->name + " ";
	if (ImGui::IsItemVisible()) {
		formatted_name += p_entity->HasComponent<MeshComponent>() ? " " ICON_FA_BOX : "";
		formatted_name += p_entity->HasComponent<PhysicsComponent>() || p_entity->HasComponent<PhysicsComponent>() ? " " ICON_FA_BEZIER_CURVE : "";
		formatted_name += p_entity->HasComponent<PointLightComponent>() ? " " ICON_FA_LIGHTBULB : "";
		formatted_name += p_entity->HasComponent<SpotLightComponent>() ? " " ICON_FA_LIGHTBULB : "";
		formatted_name += p_entity->HasComponent<CameraComponent>() ? " " ICON_FA_CAMERA : "";
		formatted_name += p_entity->HasComponent<ParticleEmitterComponent>() ? " " ICON_FA_SNOWFLAKE : "";
		formatted_name += p_entity->HasComponent<AudioComponent>() ? " " ICON_FA_VOLUME_HIGH : "";
		formatted_name += p_entity->HasComponent<ScriptComponent>() ? " " ICON_FA_FILE_INVOICE : "";
		formatted_name += p_entity->HasComponent<JointComponent>() ? " " ICON_FA_DIAGRAM_PROJECT : "";
	}

	auto* p_entity_relationship_comp = p_entity->GetComponent<RelationshipComponent>();
	bool is_selected = VectorContains(m_state.selected_entity_ids, p_entity->GetUUID());
	bool is_open = VectorContains(m_state.open_tree_nodes_entities, p_entity->GetUUID());

	ImVec4 tree_node_bg_col = is_selected ? m_res.lighter_grey_color : ImVec4(0, 0, 0, 0);
	ImGui::PushStyleColor(ImGuiCol_Header, tree_node_bg_col);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 3));
	ImGui::PushID(p_entity);


	ImGui::SetNextItemOpen(is_open);
	auto flags = p_entity_relationship_comp->first == entt::null ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Framed : ImGuiTreeNodeFlags_Framed;
	flags |= p_entity_relationship_comp->num_children > 0 ? ImGuiTreeNodeFlags_OpenOnArrow : ImGuiTreeNodeFlags_Bullet;
	bool is_tree_node_open = ImGui::TreeNodeEx(formatted_name.c_str(), flags);

	ret.node_screen_min = ImGui::GetItemRectMin();
	ret.node_screen_max = ImGui::GetItemRectMax();

	if (node_selection_active) {
		if (ExtraUI::DoBoxesIntersect(ret.node_screen_min, ret.node_screen_max, selection_box.min, selection_box.max))
			SelectEntity(p_entity->GetUUID());
		else if (!Window::Get().input.IsKeyDown(Key::LeftControl))
			DeselectEntity(p_entity->GetUUID());
	}

	if (ImGui::IsItemToggledOpen()) {
		auto it = std::ranges::find(m_state.open_tree_nodes_entities, p_entity->GetUUID());
		if (it == m_state.open_tree_nodes_entities.end())
			m_state.open_tree_nodes_entities.push_back(p_entity->GetUUID());
		else
			m_state.open_tree_nodes_entities.erase(it);
	}

	if (ImGui::BeginDragDropTarget()) {
		if (ImGui::AcceptDragDropPayload("ENTITY")) {
			/* Functionality handled externally */
		}
		ImGui::EndDragDropTarget();
	}

	m_state.selected_entities_are_dragged |= ImGui::IsItemHovered() && ImGui::IsMouseDragging(0);

	if (m_state.selected_entities_are_dragged) {
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceExtern)) {
			ImGui::SetDragDropPayload("ENTITY", &m_state.selected_entity_ids, sizeof(std::vector<uint64_t>));
			ImGui::EndDragDropSource();
		}
	}

	// Drag entities into another entity node to make them children of it
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && m_state.selected_entities_are_dragged && ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax())) {
		DeselectEntity(p_entity->GetUUID());
		for (auto id : m_state.selected_entity_ids) {
			SCENE->GetEntity(id)->SetParent(*p_entity);
		}

		m_state.selected_entities_are_dragged = false;
	}

	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

	std::string popup_id = std::format("{}", p_entity->GetUUID()); // unique popup id
	if (ImGui::IsItemHovered()) {
		// Right click to open popup
		if (ImGui::IsMouseClicked(1))
			ImGui::OpenPopup(popup_id.c_str());

		if (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1)) {
			if (!Window::Get().input.IsKeyDown(GLFW_KEY_LEFT_CONTROL)) // Only selecting one entity at a time
				m_state.selected_entity_ids.clear();

			if (Window::Get().input.IsKeyDown(GLFW_KEY_LEFT_CONTROL) && VectorContains(m_state.selected_entity_ids, p_entity->GetUUID()) && ImGui::IsMouseClicked(0)) // Deselect entity from group of entities currently selected
				m_state.selected_entity_ids.erase(std::ranges::find(m_state.selected_entity_ids, p_entity->GetUUID()));
			else
				SelectEntity(p_entity->GetUUID());
		}

	}

	// Popup opened above if node is right clicked
	if (ImGui::BeginPopup(popup_id.c_str()))
	{
		ImGui::SeparatorText("Options");
		if (ImGui::Selectable("Delete"))
			ret.e_event = EntityNodeEvent::E_DELETE;

		// Creates clone of entity, puts it in scene
		if (ImGui::Selectable("Duplicate"))
			ret.e_event = EntityNodeEvent::E_DUPLICATE;

		ImGui::EndPopup();
	}

	if (is_tree_node_open) {
		ImGui::TreePop(); // Pop tree node opened earlier
	}

	ImGui::PopID();
	return ret;
}


void EditorLayer::RenderSceneGraph() {
	if (ImGui::BeginChild("Scene graph", { static_cast<float>(RIGHT_WINDOW_WIDTH), static_cast<float>(Window::GetHeight() - m_res.toolbar_height) * 0.5f }, true)) {
		ImGui::Text("Scene name:"); ImGui::SameLine(); ImGui::InputText("##scene-name-input", &SCENE->m_name);

		// Click anywhere on window to deselect entity nodes
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !Window::Get().input.IsKeyDown(GLFW_KEY_LEFT_CONTROL))
			m_state.selected_entity_ids.clear();

		// Right click to bring up "new entity" popup
		RenderCreationWidget(nullptr, ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1));

		bool node_selection_active = ImGui::IsWindowHovered() && ImGui::IsMouseDragging(2);
		Box2D node_selection_box{};

		if (node_selection_active) {
			auto start_pos = ImGui::GetIO().MouseClickedPos[2];
			auto end_pos = ImGui::GetIO().MousePos;
			node_selection_box.min = glm::vec2(glm::min(start_pos.x, end_pos.x), glm::min(start_pos.y, end_pos.y));
			node_selection_box.max = glm::vec2(glm::max(start_pos.x, end_pos.x), glm::max(start_pos.y, end_pos.y));

			ImGui::SetNextWindowPos({ node_selection_box.min.x, node_selection_box.min.y });
			ImGui::SetNextWindowSize({ node_selection_box.max.x - node_selection_box.min.x, node_selection_box.max.y - node_selection_box.min.y });
			if (ImGui::Begin("##sel-box", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs)) {
				ImGui::PushStyleColor(ImGuiCol_Button, { 0.f, 0.f, 1.f, 0.1f });
				ImGui::Button("##blue", { node_selection_box.max.x - node_selection_box.min.x,  node_selection_box.max.y - node_selection_box.min.y });
				ImGui::PopStyleColor();
			}
			ImGui::End();
		}

		EntityNodeEvent active_event = EntityNodeEvent::E_NONE;

		if (ExtraUI::H2TreeNode("Scene settings")) {
			m_state.general_settings.editor_window_settings.display_skybox_editor = ExtraUI::EmptyTreeNode("Skybox");
			m_state.general_settings.editor_window_settings.display_directional_light_editor = ExtraUI::EmptyTreeNode("Directional Light");
			m_state.general_settings.editor_window_settings.display_global_fog_editor = ExtraUI::EmptyTreeNode("Global fog");
			m_state.general_settings.editor_window_settings.display_bloom_editor = ExtraUI::EmptyTreeNode("Bloom");
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2);
		ImGui::PushStyleColor(ImGuiCol_Header, m_res.lightest_grey_color);
		if (ExtraUI::H2TreeNode("Entities")) {
			std::vector<EditorLayer::EntityNodeEntry> entity_graph;
			// TODO: This should only be updated when the scene graph changes
			GetEntityGraph(entity_graph);

			for (auto& entry : entity_graph) {
				auto node_data = RenderEntityNode(entry.p_entity, entry.depth, node_selection_active, node_selection_box);
				active_event = static_cast<EntityNodeEvent>(node_data.e_event | active_event);
			}
		}
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();

		// Process node events
		if (active_event & EntityNodeEvent::E_DUPLICATE) {
			DuplicateEntitiesTracked(m_state.selected_entity_ids);
		}
		else if (active_event & EntityNodeEvent::E_DELETE) {
			for (auto id : m_state.selected_entity_ids) {
				if (SCENE->GetEntity(id))
					DeleteEntitiesTracked(m_state.selected_entity_ids);
			}
		}


		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			if (m_state.selected_entities_are_dragged && ImGui::IsWindowHovered()) {
				for (auto id : m_state.selected_entity_ids) {
					SCENE->GetEntity(id)->RemoveParent();
				}
			}

			m_state.selected_entities_are_dragged = false;
		}
	}
	ImGui::EndChild();
}


template<std::derived_from<Component> CompType>
void RenderCompEditor(SceneEntity* p_entity, const std::string& comp_name, std::function<void(CompType*)> render_func) {

	if (auto* p_comp = p_entity->GetComponent<CompType>(); p_comp && ImGui::CollapsingHeader(comp_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(1)) {
			if constexpr (!std::is_same_v<CompType, TransformComponent>)
				p_entity->DeleteComponent<CompType>();
		}
		else {
			render_func(p_comp);
		}
	}
}



void EditorLayer::DisplayEntityEditor() {
	if (ImGui::BeginChild("Entity editor", { static_cast<float>(RIGHT_WINDOW_WIDTH), static_cast<float>(Window::GetHeight() - m_res.toolbar_height) * 0.5f }, true)) {
		if (m_state.general_settings.editor_window_settings.display_directional_light_editor)
			RenderDirectionalLightEditor();
		if (m_state.general_settings.editor_window_settings.display_global_fog_editor)
			RenderGlobalFogEditor();
		if (m_state.general_settings.editor_window_settings.display_skybox_editor)
			RenderSkyboxEditor();
		if (m_state.general_settings.editor_window_settings.display_bloom_editor)
			RenderBloomEditor();

		auto entity = SCENE->GetEntity(m_state.selected_entity_ids.empty() ? 0 : m_state.selected_entity_ids[0]);
		if (!entity) {
			ImGui::EndChild();
			return;
		}

		static std::vector<TransformComponent*> transforms;
		transforms.clear();
		for (auto id : m_state.selected_entity_ids) {
			transforms.push_back(SCENE->GetEntity(id)->GetComponent<TransformComponent>());
		}

		ImGui::PushFont(m_res.p_l_font);
		ExtraUI::AlphaNumTextInput(entity->name);
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - 55);
		RenderCreationWidget(entity, ImGui::Button("+"));
		ImGui::PopFont();

		RenderCompEditor<TransformComponent>(entity, "Transform component", [this]([[maybe_unused]] TransformComponent* p_comp) { RenderTransformComponentEditor(transforms); });
		RenderCompEditor<MeshComponent>(entity, "Mesh component", [this](MeshComponent* p_comp) { RenderMeshComponentEditor(p_comp); });
		RenderCompEditor<PointLightComponent>(entity, "Pointlight component", [this](PointLightComponent* p_comp) { RenderPointlightEditor(p_comp); });
		RenderCompEditor<SpotLightComponent>(entity, "Spotlight component", [this](SpotLightComponent* p_comp) { RenderSpotlightEditor(p_comp); });
		RenderCompEditor<CameraComponent>(entity, "Camera component", [this](CameraComponent* p_comp) { RenderCameraEditor(p_comp); });
		RenderCompEditor<PhysicsComponent>(entity, "Physics component", [this](PhysicsComponent* p_comp) { RenderPhysicsComponentEditor(p_comp); });
		RenderCompEditor<ScriptComponent>(entity, "Script component", [this](ScriptComponent* p_comp) { RenderScriptComponentEditor(p_comp); });
		RenderCompEditor<AudioComponent>(entity, "Audio component", [this](AudioComponent* p_comp) { RenderAudioComponentEditor(p_comp); });
		RenderCompEditor<ParticleEmitterComponent>(entity, "Particle emitter component", [this](ParticleEmitterComponent* p_comp) { RenderParticleEmitterComponentEditor(p_comp); });
		RenderCompEditor<ParticleBufferComponent>(entity, "Particle buffer component", [this](ParticleBufferComponent* p_comp) { RenderParticleBufferComponentEditor(p_comp); });

		glm::vec2 window_size = { ImGui::GetWindowSize().x, ImGui::GetWindowSize().y };
		glm::vec2 button_size = { 200, 50 };
		glm::vec2 padding_size = { (window_size.x / 2.f) - button_size.x / 2.f, 50.f };
		ImGui::Dummy(ImVec2(padding_size.x, padding_size.y));
	}

	ImGui::EndChild();
	ImGui::Dummy({ 0, 50 });
}


template<typename T>
T* AnyCast(std::any& a) {
	try {
		return std::any_cast<T*>(a);
	}
	catch (std::exception e) {
		return nullptr;
	}
}

void EditorLayer::RenderScriptComponentEditor(ScriptComponent* p_script) {
	ImGui::PushID(p_script);

	ExtraUI::NameWithTooltip(p_script->GetSymbols() ? p_script->GetSymbols()->script_name : "No script asset");
	ImGui::Button(ICON_FA_FILE, ImVec2(100, 100));
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("SCRIPT")) {
			if (p_payload->DataSize == sizeof(ScriptAsset*)) {
				ScriptSymbols* p_symbols = &(*static_cast<ScriptAsset**>(p_payload->Data))->symbols;
				p_script->SetSymbols(p_symbols);
			}
		}
		ImGui::EndDragDropTarget();
	}

	ImGui::PopID(); // p_script
}


void EditorLayer::RenderParticleBufferComponentEditor(ParticleBufferComponent* p_comp) {
	ImGui::PushID(p_comp);

	ImGui::Text("Min. allocated particles"); ImGui::SameLine();
	if (ExtraUI::InputUint("##e", p_comp->m_min_allocated_particles)) {
		p_comp->DispatchUpdateEvent();
	}

	ImGui::Text("Buffer ID"); ImGui::SameLine();
	if (ExtraUI::InputUint("##bi", p_comp->m_buffer_id)) {
		p_comp->DispatchUpdateEvent();
	}

	ImGui::PopID();

}

void EditorLayer::RenderParticleEmitterComponentEditor(ParticleEmitterComponent* p_comp) {
	ImGui::PushID(p_comp);

	const char* types[2] = { "BILLBOARD", "MESH" };
	ParticleEmitterComponent::EmitterType emitter_types[2] = { ParticleEmitterComponent::EmitterType::BILLBOARD, ParticleEmitterComponent::EmitterType::MESH };
	static int current_item = 0;
	current_item = p_comp->m_type;

	ImGui::Text("Render type"); ImGui::SameLine();
	if (ImGui::BeginCombo("##type selection", types[current_item])) {
		for (int i = 0; i < 2; i++) {
			bool selected = current_item == i;

			if (ImGui::Selectable(types[i], selected)) {
				current_item = i;
				p_comp->SetType(emitter_types[i]);
			}

			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	ImGui::SeparatorText("Resources");

	if (p_comp->m_type == ParticleEmitterComponent::MESH) {
		auto* p_res = p_comp->GetEntity()->GetComponent<ParticleMeshResources>();

		std::function<void(MeshAsset* p_new)> OnMeshDrop = [p_res](MeshAsset* p_new) {
			p_res->p_mesh = p_new;
			p_res->materials = { p_new->GetNbMaterials(), AssetManager::GetAsset<Material>(static_cast<uint64_t>(BaseAssetIDs::DEFAULT_MATERIAL)) };
			};

		std::function<void(unsigned index, Material* p_new)> OnMaterialDrop = [p_res](unsigned index, Material* p_new) {
			p_res->materials[index] = p_new;
			};

		RenderMeshWithMaterials(p_res->p_mesh, p_res->materials, OnMeshDrop, OnMaterialDrop);
	}
	else {
		auto* p_res = p_comp->GetEntity()->GetComponent<ParticleBillboardResources>();
		if (auto* p_new = RenderMaterialComponent(p_res->p_material)) {
			p_res->p_material = p_new;
		}
	}

	ImGui::SeparatorText("Modifiers");

	if (ImGui::TreeNode("Colour over life")) {
		if (ExtraUI::InterpolatorV3Graph(&p_comp->m_life_colour_interpolator))
			p_comp->DispatchUpdateEvent(ParticleEmitterComponent::MODIFIERS_CHANGED);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Alpha over life")) {
		if (ExtraUI::InterpolatorV1Graph(&p_comp->m_life_alpha_interpolator))
			p_comp->DispatchUpdateEvent(ParticleEmitterComponent::MODIFIERS_CHANGED);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Scale over life")) {
		if (ExtraUI::InterpolatorV3Graph(&p_comp->m_life_scale_interpolator))
			p_comp->DispatchUpdateEvent(ParticleEmitterComponent::MODIFIERS_CHANGED);

		ImGui::TreePop();
	}


	ImGui::Text("Acceleration"); ImGui::SameLine();
	if (ImGui::DragFloat3("##ac", &p_comp->acceleration.x)) {
		p_comp->SetAcceleration(p_comp->acceleration);
	}

	ImGui::SeparatorText("Parameters");

	ImGui::Text("Spread"); ImGui::SameLine();
	if (ImGui::DragFloat("##spread", &p_comp->m_spread, 1.f, 0.f, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
		p_comp->SetSpread(p_comp->m_spread);
	}

	ImGui::Text("Lifespan"); ImGui::SameLine();
	if (ImGui::DragFloat("##ls", &p_comp->m_particle_lifespan_ms, 1.f, 0.f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
		p_comp->SetParticleLifespan(p_comp->m_particle_lifespan_ms);
	}


	ImGui::Text("Spawn delay"); ImGui::SameLine();
	if (ImGui::DragFloat("##sd", &p_comp->m_particle_spawn_delay_ms, 1.f, 0.f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
		p_comp->SetParticleSpawnDelay(p_comp->m_particle_spawn_delay_ms);
	}

	ImGui::Text("Nb. particles"); ImGui::SameLine();
	static int n;
	n = static_cast<int>(p_comp->m_num_particles);
	if (ImGui::DragInt("##np", &n) && n >= 0 && n <= 100'000) {
		p_comp->SetNbParticles(n);
	}

	ImGui::Text("Active"); ImGui::SameLine();
	if (ImGui::Checkbox("##active", &p_comp->m_active)) {
		p_comp->SetActive(p_comp->m_active);
	}

	if (ExtraUI::ShowVec3Editor("Spawn extents", p_comp->m_spawn_extents)) {
		p_comp->SetSpawnExtents(p_comp->m_spawn_extents);
	}

	if (ExtraUI::ShowVec2Editor("Velocity scale range", p_comp->m_velocity_min_max_scalar)) {
		p_comp->SetVelocityScale(p_comp->m_velocity_min_max_scalar);
	}

	ImGui::PopID();
}


void EditorLayer::RenderAudioComponentEditor(AudioComponent* p_audio) {
	ImGui::PushID(p_audio);

	ImGui::PushItemWidth(200.f);
	static float volume = p_audio->GetVolume();
	volume = p_audio->GetVolume();
	ImGui::Text("Volume");
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 300.f);
	if (ImGui::InputFloat("##volume", &volume)) {
		p_audio->SetVolume(volume);
	}

	static float pitch = p_audio->GetPitch();
	pitch = p_audio->GetPitch();
	ImGui::Text("Pitch");
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 300.f);
	if (ImGui::InputFloat("##pitch", &pitch)) {
		p_audio->SetPitch(pitch);
	}

	auto range = p_audio->GetMinMaxRange();
	static float min_range = range.min;
	min_range = range.min;
	static float max_range = range.max;
	max_range = range.max;

	ImGui::Text("Min range");
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 300.f);
	if (ImGui::InputFloat("##min_range", &min_range)) {
		p_audio->SetMinMaxRange(min_range, max_range);
	}
	ImGui::Text("Max range");
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 300.f);
	if (ImGui::InputFloat("##max_range", &max_range)) {
		p_audio->SetMinMaxRange(min_range, max_range);
	}
	ImGui::PopItemWidth();

	ImGui::Separator();

	auto* p_sound = AssetManager::GetAsset<SoundAsset>(p_audio->m_sound_asset_uuid);
	ImVec2 prev_curs_pos = ImGui::GetCursorPos();
	constexpr unsigned playback_widget_width = 400;
	constexpr unsigned playback_widget_height = 20;
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10);
	ExtraUI::ColoredButton("##playback bg", ImVec4{ 0.25, 0.25, 0.25, 1 }, ImVec2(playback_widget_width, playback_widget_height));

	unsigned int position;
	p_audio->mp_channel->getPosition(&position, FMOD_TIMEUNIT_MS);
	unsigned int total_length;
	p_sound->p_sound->getLength(&total_length, FMOD_TIMEUNIT_MS);

	ImGui::SetCursorPos(prev_curs_pos);

	ExtraUI::ColoredButton("##playback position", m_res.orange_color_dark,
		ImVec2{
			static_cast<float>(playback_widget_width) * (static_cast<float>(position) / static_cast<float>(total_length)),
			static_cast<float>(playback_widget_height)
		}
	);

	ImGui::PopStyleVar();
	ImGui::SetCursorPos(prev_curs_pos);
	ImGui::Text("%s", std::format("{}:{}", static_cast<float>(position) / 1000.f, static_cast<float>(total_length) / 1000.f).c_str());
	ImVec2 wp = ImGui::GetWindowPos();

	static bool was_paused = true;
	static bool first_mouse_down = false;
	if (ImGui::IsMouseHoveringRect(ImVec2{ wp.x + prev_curs_pos.x, wp.y + prev_curs_pos.y }, ImVec2{ wp.x + prev_curs_pos.x + playback_widget_width, wp.y + prev_curs_pos.y + playback_widget_height })) {
		if (ImGui::IsMouseDown(0)) {
			// Channel needs to be playing to set playback position
			if (!p_audio->IsPlaying()) {
				p_audio->Play();
				p_audio->SetPaused(true);
			}

			if (!first_mouse_down) {
				was_paused = p_audio->IsPaused();
				first_mouse_down = true;
				p_audio->SetPaused(true);
			}


			ImVec2 mouse_pos = ImGui::GetMousePos();
			ImVec2 local_mouse{ mouse_pos.x - (wp.x + prev_curs_pos.x), mouse_pos.y - (wp.y + prev_curs_pos.y) };

			p_audio->mp_channel->setPosition(
				static_cast<unsigned>((local_mouse.x / static_cast<float>(playback_widget_width)) * static_cast<float>(total_length)),
				FMOD_TIMEUNIT_MS
			);
		}
	}
	if (first_mouse_down && ImGui::IsMouseReleased(0)) {
		first_mouse_down = false;
		p_audio->SetPaused(was_paused);
	}

	if (!p_audio->IsPlaying() && ImGui::SmallButton(ICON_FA_PLAY)) {
		p_audio->Play();
	}

	if (p_audio->IsPlaying()) {
		if (!p_audio->IsPaused() && ImGui::SmallButton(ICON_FA_PAUSE)) {
			p_audio->SetPaused(true);
		}
		if (p_audio->IsPaused() && ImGui::SmallButton(ICON_FA_PLAY)) {
			p_audio->SetPaused(false);
		}
	}
	ImGui::SameLine();
	if (ImGui::SmallButton(ICON_FA_REPEAT)) {
		p_audio->Stop();
		p_audio->Play(p_audio->GetAudioAssetUUID());
	}

	ImGui::Button(ICON_FA_MUSIC, ImVec2{ 100, 100 });
	if (ImGui::BeginItemTooltip()) {
		ImGui::Text("Drop a sound asset here");
		ImGui::EndTooltip();
	}
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("AUDIO"); p_payload && p_payload->DataSize == sizeof(SoundAsset*)) {
			p_audio->SetSoundAssetUUID((*static_cast<SoundAsset**>(p_payload->Data))->uuid());
		}
	}

	ImGui::Text("%s", p_sound->filepath.substr(p_sound->filepath.rfind("/") + 1).c_str());

	ImGui::PopID(); // p_audio
}


void EditorLayer::RenderPhysicsComponentEditor(PhysicsComponent* p_comp) {
	if (ImGui::BeginTable("##layout", 2, ImGuiTableFlags_Borders)) {
		ImGui::TableNextColumn();

		ImGui::SeparatorText("Collider geometry");

		if (ImGui::RadioButton("Box", p_comp->m_geometry_type == PhysicsComponent::BOX)) {
			p_comp->UpdateGeometry(PhysicsComponent::BOX);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Sphere", p_comp->m_geometry_type == PhysicsComponent::SPHERE)) {
			p_comp->UpdateGeometry(PhysicsComponent::SPHERE);
		}

		/*ImGui::SameLine(); // TODO Add back when shape assets added
		if (ImGui::RadioButton("Mesh", p_comp->m_geometry_type == PhysicsComponent::TRIANGLE_MESH)) {
			p_comp->UpdateGeometry(PhysicsComponent::TRIANGLE_MESH);
		}*/

		ImGui::SeparatorText("Body type");
		if (ImGui::RadioButton("Dynamic", p_comp->m_body_type == PhysicsComponent::DYNAMIC)) {
			p_comp->SetBodyType(PhysicsComponent::DYNAMIC);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Static", p_comp->m_body_type == PhysicsComponent::STATIC)) {
			p_comp->SetBodyType(PhysicsComponent::STATIC);
		}

		ImGui::Spacing();

		static bool is_trigger = false;
		is_trigger = p_comp->IsTrigger();
		if (ImGui::Checkbox("Trigger", &is_trigger)) {
			p_comp->SetTrigger(is_trigger);
		}

		ImGui::TableNextColumn();
		ImGui::SeparatorText("Material");
		ImGui::Button(ICON_FA_FILE, { 125, 125 });

		ImGui::EndTable();
	}
}



void EditorLayer::RenderTransformComponentEditor(std::vector<TransformComponent*>& transforms) {
	static bool render_gizmos = true;

	ImGui::Checkbox("Gizmos", &render_gizmos);
	ImGui::SameLine();
	static bool absolute_mode = false;
	absolute_mode = transforms[0]->IsAbsolute();
	if (ImGui::Checkbox("Absolute", &absolute_mode)) {
		transforms[0]->SetAbsoluteMode(absolute_mode);
	}

	glm::vec3 matrix_translation = transforms[0]->m_pos;
	glm::vec3 matrix_rotation = transforms[0]->GetOrientation();
	glm::vec3 matrix_scale = transforms[0]->m_scale;

	// UI section
	if (ExtraUI::ShowVec3Editor("T", matrix_translation))
		std::ranges::for_each(transforms, [matrix_translation](TransformComponent* p_transform) {p_transform->SetPosition(matrix_translation); });

	if (ExtraUI::ShowVec3Editor("R", matrix_rotation))
		std::ranges::for_each(transforms, [matrix_rotation](TransformComponent* p_transform) {p_transform->SetOrientation(matrix_rotation); });

	if (ExtraUI::ShowVec3Editor("S", matrix_scale))
		std::ranges::for_each(transforms, [matrix_scale](TransformComponent* p_transform) {p_transform->SetScale(matrix_scale); });


	if (!render_gizmos)
		return;

	// Gizmos
	ImGuizmo::BeginFrame();
	if (m_state.fullscreen_scene_display)
		ImGuizmo::SetRect(ImGui::GetMainViewport()->Pos.x, ImGui::GetMainViewport()->Pos.y + m_res.toolbar_height, m_state.scene_display_rect.x, m_state.scene_display_rect.y);
	else
		ImGuizmo::SetRect(ImGui::GetMainViewport()->Pos.x, ImGui::GetMainViewport()->Pos.y + m_res.toolbar_height, m_state.scene_display_rect.x, m_state.scene_display_rect.y);

	if (m_state.selection_mode != SelectionMode::ENTITY)
		return;

	glm::mat4 current_operation_matrix = transforms[0]->GetMatrix();

	CameraComponent* p_cam = SCENE->GetSystem<CameraSystem>().GetActiveCamera();
	auto* p_cam_transform = p_cam->GetEntity()->GetComponent<TransformComponent>();
	glm::vec3 cam_pos = p_cam_transform->GetAbsPosition();
	glm::mat4 view_mat = glm::lookAt(cam_pos, cam_pos + p_cam_transform->forward, p_cam_transform->up);

	glm::mat4 delta_matrix;
	ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
	static glm::vec3 snap = glm::vec3(0.01f);

	static bool is_using = false;
	static bool mouse_down = false;

	if (ImGuizmo::Manipulate(&view_mat[0][0], &p_cam->GetProjectionMatrix()[0][0], m_state.current_gizmo_operation, m_state.current_gizmo_mode,
		&current_operation_matrix[0][0], &delta_matrix[0][0], &snap[0]) && ImGuizmo::IsUsing())
		{
		if (!is_using && !mouse_down && !m_state.simulate_mode_active) {
			EditorEntityEvent e{ TRANSFORM_UPDATE, m_state.selected_entity_ids };
			for (auto id : m_state.selected_entity_ids) {
				e.serialized_entities_before.push_back(SceneSerializer::SerializeEntityIntoString(*SCENE->GetEntity(id)));
			}
			m_event_stack.PushEvent(e);
		}

		ImGuizmo::DecomposeMatrixToComponents(&delta_matrix[0][0], &matrix_translation[0], &matrix_rotation[0], &matrix_scale[0]);

		// The origin of the transform (rotate about this point)
		glm::vec3 base_abs_translation = transforms[0]->GetAbsPosition();

		glm::vec3 delta_translation = matrix_translation;
		glm::vec3 delta_scale = matrix_scale;
		glm::vec3 delta_rotation = matrix_rotation;

		static std::vector<glm::vec3> scale_dividers;

		for (auto* p_transform : transforms) {
			switch (m_state.current_gizmo_operation) {
			case ImGuizmo::TRANSLATE:
				p_transform->SetAbsolutePosition(p_transform->GetAbsPosition() + delta_translation);
				break;
			case ImGuizmo::SCALE:
				p_transform->SetScale(p_transform->m_scale * delta_scale);
				break;
			case ImGuizmo::ROTATE: {
				// This will rotate multiple objects as one, using entity transform at m_state.selected_entity_ids[0] as origin
				if (auto* p_parent_transform = p_transform->GetParent()) {
					glm::vec3 s = p_parent_transform->GetAbsScale();
					glm::mat4 rot = glm::mat4(glm::inverse(ExtraMath::Init3DScaleTransform(s.x, s.y, s.z)));
					rot[3][3] = 1.0;

					glm::mat3 to_parent_space = p_parent_transform->GetMatrix() * rot;
					glm::vec3 local_rot = glm::inverse(to_parent_space) * glm::vec4(delta_rotation, 0.0);
					glm::vec3 total = glm::eulerAngles(glm::quat(glm::radians(local_rot)) * p_transform->m_orientation);
					p_transform->SetOrientation(glm::degrees(total));
				}
				else {
					auto orientation = glm::degrees(glm::eulerAngles(glm::quat(glm::radians(delta_rotation)) * p_transform->m_orientation));
					p_transform->SetOrientation(orientation.x, orientation.y, orientation.z);
				}

				glm::vec3 abs_translation = p_transform->GetAbsPosition();
				glm::vec3 transformed_pos = abs_translation - base_abs_translation;
				glm::vec3 rotation_offset = glm::mat3(ExtraMath::Init3DRotateTransform(delta_rotation.x, delta_rotation.y, delta_rotation.z)) * transformed_pos; // rotate around transformed origin
				p_transform->SetAbsolutePosition(base_abs_translation + rotation_offset);
				break;
			}
			case ImGuizmo::TRANSLATE_X:
			case ImGuizmo::TRANSLATE_Y:
			case ImGuizmo::TRANSLATE_Z:
			case ImGuizmo::ROTATE_X:
			case ImGuizmo::ROTATE_Y:
			case ImGuizmo::ROTATE_Z:
			case ImGuizmo::ROTATE_SCREEN:
			case ImGuizmo::SCALE_X:
			case ImGuizmo::SCALE_Y:
			case ImGuizmo::SCALE_Z:
			case ImGuizmo::BOUNDS:
			case ImGuizmo::SCALE_XU:
			case ImGuizmo::SCALE_YU:
			case ImGuizmo::SCALE_ZU:
			case ImGuizmo::SCALEU:
			case ImGuizmo::UNIVERSAL:
				break;
			}
		}

		is_using = true;
		mouse_down = Window::Get().input.IsMouseDown(0);
	}
	else {
		if (ImGui::IsMouseReleased(0)) {
			mouse_down = false;
			is_using = false;
		}
	}
}




Material* EditorLayer::RenderMaterialComponent(const Material* p_material) {
	Material* ret = nullptr;

	if (ImGui::ImageButton(reinterpret_cast<void*>(m_asset_manager_window.GetMaterialPreviewTex(p_material)), ImVec2(100, 100), ImVec2(0, 1), ImVec2(1, 0))) {
		m_asset_manager_window.SelectMaterial(AssetManager::GetAsset<Material>(p_material->uuid()));
	}

	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("MATERIAL")) {
			if (p_payload->DataSize == sizeof(Material*))
				ret = *static_cast<Material**>(p_payload->Data);
		}
		ImGui::EndDragDropTarget();
	}

	ImGui::Text("%s", p_material->name.c_str());
	return ret;
}




void EditorLayer::RenderMeshWithMaterials(const MeshAsset* p_asset, std::vector<const Material*>& materials, std::function<void(MeshAsset* p_new)> OnMeshDrop, std::function<void(unsigned index, Material* p_new)> OnMaterialDrop) {
	ImGui::PushID(p_asset);

	ImGui::SeparatorText("Mesh");
	ImGui::ImageButton(reinterpret_cast<void*>(m_asset_manager_window.GetMeshPreviewTex(p_asset)), ImVec2(100, 100), ImVec2(0, 1), ImVec2(1, 0));
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("MESH")) {
			if (p_payload->DataSize == sizeof(MeshAsset*)) {
				OnMeshDrop(*static_cast<MeshAsset**>(p_payload->Data));
			}
		}
		ImGui::EndDragDropTarget();
	}

	ImGui::SeparatorText("Materials");
	for (size_t i = 0; i < materials.size(); i++) {
		auto p_material = materials[i];
		ImGui::PushID(static_cast<int>(i));

		if (auto* p_new_material = RenderMaterialComponent(p_material)) {
			OnMaterialDrop(static_cast<unsigned>(i), p_new_material);
		}

		ImGui::PopID();
	}


	ImGui::PopID();
}

void EditorLayer::RenderMeshComponentEditor(MeshComponent* comp) {
	ImGui::PushID(comp);

	std::function<void(MeshAsset* p_new)> OnMeshDrop = [comp](MeshAsset* p_new) {
		comp->SetMeshAsset(p_new);
		};

	std::function<void(unsigned index, Material* p_new)> OnMaterialDrop = [comp](unsigned index, Material* p_new) {
		comp->SetMaterialID(index, p_new);
		};

	RenderMeshWithMaterials(comp->mp_mesh_asset, comp->m_materials, OnMeshDrop, OnMaterialDrop);

	ImGui::PopID();
};




void EditorLayer::RenderSpotlightEditor(SpotLightComponent* light) {
	float aperture = glm::degrees(acosf(light->m_aperture));

	ImGui::PushItemWidth(200.f);
	ImGui::SliderFloat("constant", &light->attenuation.constant, 0.0f, 1.0f);
	ImGui::SliderFloat("linear", &light->attenuation.linear, 0.0f, 1.0f);
	ImGui::SliderFloat("exp", &light->attenuation.exp, 0.0f, 0.1f);
	ImGui::Checkbox("Shadows", &light->shadows_enabled);
	ImGui::SliderFloat("Shadow distance", &light->shadow_distance, 0.0f, 5000.0f);

	ImGui::Text("Aperture");
	ImGui::SameLine();
	ImGui::SliderFloat("##a", &aperture, 0.f, 90.f);

	ImGui::PopItemWidth();

	ExtraUI::ShowColorVec3Editor("Colour", light->colour);

	light->SetAperture(aperture);
}




void EditorLayer::RenderGlobalFogEditor() {
	if (ExtraUI::H2TreeNode("Global fog")) {
		ImGui::Text("Scattering");
		ImGui::SliderFloat("##scattering", &SCENE->post_processing.global_fog.scattering_coef, 0.f, 0.1f);
		ImGui::Text("Absorption");
		ImGui::SliderFloat("##absorption", &SCENE->post_processing.global_fog.absorption_coef, 0.f, 0.1f);
		ImGui::Text("Density");
		ImGui::SliderFloat("##density", &SCENE->post_processing.global_fog.density_coef, 0.f, 1.f);
		ImGui::Text("Scattering anistropy");
		ImGui::SliderFloat("##scattering anistropy", &SCENE->post_processing.global_fog.scattering_anisotropy, -1.f, 1.f);
		ImGui::Text("Emissive factor");
		ImGui::SliderFloat("##emissive", &SCENE->post_processing.global_fog.emissive_factor, 0.f, 2.f);
		ImGui::Text("Step count");
		ImGui::SliderInt("##step count", &SCENE->post_processing.global_fog.step_count, 0, 512);
		ExtraUI::ShowColorVec3Editor("Colour", SCENE->post_processing.global_fog.colour);
	}
}




void EditorLayer::RenderBloomEditor() {
	if (ExtraUI::H2TreeNode("Bloom")) {
		ImGui::SliderFloat("Intensity", &SCENE->post_processing.bloom.intensity, 0.f, 10.f);
		ImGui::SliderFloat("Threshold", &SCENE->post_processing.bloom.threshold, 0.0f, 50.0f);
		ImGui::SliderFloat("Knee", &SCENE->post_processing.bloom.knee, 0.f, 1.f);
	}
}



void EditorLayer::RenderDirectionalLightEditor() {
	if (ExtraUI::H1TreeNode("Directional light")) {
		ImGui::Text("DIR LIGHT CONTROLS");

		static glm::vec3 light_dir = SCENE->directional_light.m_light_direction;
		static glm::vec3 light_colour = SCENE->directional_light.colour;

		ImGui::SliderFloat("X", &light_dir.x, -1.f, 1.f);
		ImGui::SliderFloat("Y", &light_dir.y, -1.f, 1.f);
		ImGui::SliderFloat("Z", &light_dir.z, -1.f, 1.f);
		ExtraUI::ShowColorVec3Editor("Color", light_colour);

		ImGui::Text("Cascade ranges");
		ImGui::SliderFloat("##c1", &SCENE->directional_light.cascade_ranges[0], 0.f, 50.f);
		ImGui::SliderFloat("##c2", &SCENE->directional_light.cascade_ranges[1], 0.f, 150.f);
		ImGui::SliderFloat("##c3", &SCENE->directional_light.cascade_ranges[2], 0.f, 500.f);
		ImGui::Text("Z-mults");
		ImGui::SliderFloat("##z1", &SCENE->directional_light.z_mults[0], 0.f, 10.f);
		ImGui::SliderFloat("##z2", &SCENE->directional_light.z_mults[1], 0.f, 10.f);
		ImGui::SliderFloat("##z3", &SCENE->directional_light.z_mults[2], 0.f, 10.f);

		ImGui::SliderFloat("Size", &SCENE->directional_light.light_size, 0.f, 150.f);
		ImGui::SliderFloat("Blocker search size", &SCENE->directional_light.blocker_search_size, 0.f, 50.f);

		ImGui::Checkbox("Shadows", &SCENE->directional_light.shadows_enabled);

		SCENE->directional_light.colour = glm::vec3(light_colour.x, light_colour.y, light_colour.z);
		SCENE->directional_light.SetLightDirection(light_dir);
	}
}




void EditorLayer::RenderPointlightEditor(PointLightComponent* light) {
	ImGui::PushItemWidth(200.f);
	ImGui::SliderFloat("constant", &light->attenuation.constant, 0.0f, 1.0f);
	ImGui::SliderFloat("linear", &light->attenuation.linear, 0.0f, 1.0f);
	ImGui::SliderFloat("exp", &light->attenuation.exp, 0.0f, 0.1f);
	ImGui::Checkbox("Shadows", &light->shadows_enabled);
	ImGui::SliderFloat("Shadow distance", &light->shadow_distance, 0.0f, 5000.0f);

	ImGui::PopItemWidth();
	ExtraUI::ShowColorVec3Editor("Color", light->colour);
}




void EditorLayer::RenderCameraEditor(CameraComponent* p_cam) {
	ImGui::PushItemWidth(200.f);
	ImGui::SliderFloat("Exposure", &p_cam->exposure, 0.f, 10.f);
	ImGui::SliderFloat("FOV", &p_cam->fov, 0.f, 180.f);
	ImGui::InputFloat("ZNEAR", &p_cam->zNear);
	ImGui::InputFloat("ZFAR", &p_cam->zFar);

	if (ImGui::Button("Make active")) {
		p_cam->MakeActive();
	}
}

std::vector<SceneEntity*> EditorLayer::DuplicateEntitiesTracked(std::vector<uint64_t> entities) {
	EditorEntityEvent e;

	std::vector<SceneEntity*> ret;
	if (entities.size() == 1) {
		SceneEntity* p_dup_ent = nullptr;

		auto* p_original_ent = SCENE->GetEntity(entities[0]);

		if (!p_original_ent)
			return ret;

		if (m_state.simulate_mode_active)
			p_dup_ent = &SCENE->DuplicateEntityCallScript(*p_original_ent);
		else
			p_dup_ent = &p_original_ent->Duplicate();

		e.affected_entities.push_back(p_dup_ent->GetUUID());

		p_dup_ent->ForEachChildRecursive([&](entt::entity ent) {
			auto* p_current_ent = SCENE->GetEntity(ent);
			e.affected_entities.push_back(p_current_ent->GetUUID());
			});

		ret.push_back(p_dup_ent);

	}
	else {
		std::vector<SceneEntity*> ents;
		for (auto id : entities) {
			ents.push_back(SCENE->GetEntity(id));
		}

		auto dups = SCENE->DuplicateEntityGroup(ents);

		for (auto* p_ent : dups) {
			e.affected_entities.push_back(p_ent->GetUUID());

			p_ent->ForEachChildRecursive([&](entt::entity ent) {
				auto* p_current_ent = SCENE->GetEntity(ent);
				e.affected_entities.push_back(p_current_ent->GetUUID());
				});

			ret.push_back(p_ent);
		}
	}

	// Remove any duplicate child entities that were added from the above child search
	std::unordered_set<uint64_t> unique_entities{ e.affected_entities.begin(), e.affected_entities.end() };
	e.affected_entities.assign(unique_entities.begin(), unique_entities.end());

	// Sort entities so the most nested ones are processed first as if the parent is processed before then the child entity may not be valid
	Scene::SortEntitiesNumParents(&*SCENE, e.affected_entities, true);

	for (auto id : e.affected_entities) {
		e.serialized_entities_before.push_back(SceneSerializer::SerializeEntityIntoString(*SCENE->GetEntity(id)));
	}

	e.event_type = ENTITY_CREATE;
	m_event_stack.PushEvent(e);

	return ret;
}


void EditorLayer::DeleteEntitiesTracked(std::vector<uint64_t> entities) {
	EditorEntityEvent e;
	e.event_type = ENTITY_DELETE;

	for (auto id : entities) {
		DeselectEntity(id);
		auto* p_entity = SCENE->GetEntity(id);
		e.affected_entities.push_back(id);

		p_entity->ForEachChildRecursive([&](entt::entity ent) {
			auto* p_current_ent = SCENE->GetEntity(ent);
			e.affected_entities.push_back(p_current_ent->GetUUID());
		});

	}

	// Remove any duplicate child entities that were added from the above child search
	std::unordered_set<uint64_t> unique_entities{ e.affected_entities.begin(), e.affected_entities.end() };
	e.affected_entities.assign(unique_entities.begin(), unique_entities.end());

	// Sort entities so the most nested ones are processed first as if the parent is processed before then the child entity may not be valid
	Scene::SortEntitiesNumParents(&*SCENE, e.affected_entities, true);

	for (auto id : e.affected_entities) {
		auto& ent = *SCENE->GetEntity(id);
		e.serialized_entities_before.push_back(SceneSerializer::SerializeEntityIntoString(ent));
		SCENE->DeleteEntity(&ent);
	}

	m_event_stack.PushEvent(e);
}

SceneEntity& EditorLayer::CreateEntityTracked(const std::string& name) {
	auto& ent = SCENE->CreateEntity(name);
	EditorEntityEvent e;
	e.event_type = ENTITY_CREATE;
	e.affected_entities.push_back(ent.GetUUID());
	e.serialized_entities_before.push_back(SceneSerializer::SerializeEntityIntoString(ent));
	m_event_stack.PushEvent(e);
	return ent;
}

void EditorLayer::SelectEntity(uint64_t id) {
	if (id == 0)
		return;

	if (!VectorContains(m_state.selected_entity_ids, id))
		m_state.selected_entity_ids.push_back(id);
	else if (!m_state.selected_entity_ids.empty())
		// Makes the selected entity the first ID, which some UI components will operate on more, e.g gizmos will render on this entity now over other selected ones
		std::iter_swap(std::ranges::find(m_state.selected_entity_ids, id), m_state.selected_entity_ids.begin());

	// Open tree nodes for the selected entity to be viewed in the scene panel
	auto* p_current_parent = SCENE->GetEntity(SCENE->GetEntity(id)->GetParent());

	while (p_current_parent) {
		if (!VectorContains(m_state.open_tree_nodes_entities, p_current_parent->GetUUID()))
			m_state.open_tree_nodes_entities.push_back(p_current_parent->GetUUID());

		p_current_parent = SCENE->GetEntity(p_current_parent->GetParent());
	}

	m_state.item_selected_this_frame = true;
}

void EditorLayer::DeselectEntity(uint64_t id) {
	auto it = std::ranges::find(m_state.selected_entity_ids, id);
	if (it != m_state.selected_entity_ids.end())
		m_state.selected_entity_ids.erase(it);
}

void EditorLayer::SetScene(Scene* p_scene) {
	mp_scene_context = p_scene;
	m_asset_manager_window.SetScene(p_scene);
}

#if defined(MSVC)
#pragma warning( pop )
#endif
