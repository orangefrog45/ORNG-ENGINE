#include "pch/pch.h"


#include <glfw/glfw3.h>
#include "yaml/include/yaml-cpp/yaml.h"
#include <fmod.hpp>

#include "EditorLayer.h"
#include "../extern/Icons.h"
#include "../extern/imgui/backends/imgui_impl_opengl3.h"
#include "scene/SceneSerializer.h"
#include "util/Timers.h"
#include "assets/AssetManager.h"
#include "scripting/ScriptingEngine.h"
#include "core/Input.h"
#include "util/ExtraUI.h"
#include "components/ParticleBufferComponent.h"
#include "tracy/public/tracy/Tracy.hpp"
#include "imgui/imgui_internal.h"

constexpr unsigned LEFT_WINDOW_WIDTH = 75;
constexpr unsigned RIGHT_WINDOW_WIDTH = 650;


using namespace ORNG::Events;

#define SCENE (*mp_scene_context)

namespace ORNG {
	void EditorLayer::Init() {
		char buffer[ORNG_MAX_FILEPATH_SIZE];
		GetModuleFileName(nullptr, buffer, ORNG_MAX_FILEPATH_SIZE);
		m_state.executable_directory = buffer;
		m_state.executable_directory = m_state.executable_directory.substr(0, m_state.executable_directory.find_last_of('\\'));

		InitImGui();
		InitLua();

		m_res.line_vao.AddBuffer<VertexBufferGL<glm::vec3>>(0, GL_FLOAT, 3, GL_STREAM_DRAW);
		m_res.line_vao.Init();

		m_res.p_raymarch_shader = &Renderer::GetShaderLibrary().CreateShaderVariants("Editor raymarch");
		m_res.p_raymarch_shader->SetPath(GL_VERTEX_SHADER, m_state.executable_directory + "/res/core-res/shaders/QuadVS.glsl");
		m_res.p_raymarch_shader->SetPath(GL_FRAGMENT_SHADER, m_state.executable_directory + "/res/shaders/RaymarchFS.glsl");
		m_res.p_raymarch_shader->AddVariant((unsigned)EditorResources::RaymarchSV::CAPSULE, { "CAPSULE" }, {"u_capsule_pos", "u_capsule_height", "u_capsule_radius"});

		m_res.grid_mesh = std::make_unique<GridMesh>();
		m_res.grid_mesh->Init();
		m_res.p_grid_shader = &Renderer::GetShaderLibrary().CreateShader("grid");
		m_res.p_grid_shader->AddStage(GL_VERTEX_SHADER, m_state.executable_directory + "/res/shaders/GridVS.glsl");
		m_res.p_grid_shader->AddStage(GL_FRAGMENT_SHADER, m_state.executable_directory + "/res/shaders/GridFS.glsl");
		m_res.p_grid_shader->Init();


		m_res.p_quad_col_shader = &Renderer::GetShaderLibrary().CreateShader("quad_col");
		m_res.p_quad_col_shader->AddStage(GL_VERTEX_SHADER, m_state.executable_directory + "/res/core-res/shaders/QuadVS.glsl", { "TRANSFORM" });
		m_res.p_quad_col_shader->AddStage(GL_FRAGMENT_SHADER, m_state.executable_directory + "/res/core-res/shaders/ColorFS.glsl");
		m_res.p_quad_col_shader->Init();
		m_res.p_quad_col_shader->AddUniforms("u_scale", "u_translation", "u_color");

		m_res.p_picking_shader = &Renderer::GetShaderLibrary().CreateShader("picking");
		m_res.p_picking_shader->AddStage(GL_VERTEX_SHADER, m_state.executable_directory + "/res/core-res/shaders/TransformVS.glsl");
		m_res.p_picking_shader->AddStage(GL_FRAGMENT_SHADER, m_state.executable_directory + "/res/shaders/PickingFS.glsl");
		m_res.p_picking_shader->Init();
		m_res.p_picking_shader->AddUniforms("comp_id", "transform");

		m_res.p_highlight_shader = &Renderer::GetShaderLibrary().CreateShader("highlight");
		m_res.p_highlight_shader->AddStage(GL_VERTEX_SHADER, m_state.executable_directory + "/res/core-res/shaders/TransformVS.glsl", {"OUTLINE"});
		m_res.p_highlight_shader->AddStage(GL_FRAGMENT_SHADER, m_state.executable_directory + "/res/core-res/shaders/ColorFS.glsl");
		m_res.p_highlight_shader->Init();
		m_res.p_highlight_shader->AddUniforms("transform", "u_color", "u_scale");

		// Setting up the scene display texture
		m_res.color_render_texture_spec.format = GL_RGBA;
		m_res.color_render_texture_spec.internal_format = GL_RGBA16F;
		m_res.color_render_texture_spec.storage_type = GL_FLOAT;
		m_res.color_render_texture_spec.mag_filter = GL_NEAREST;
		m_res.color_render_texture_spec.min_filter = GL_NEAREST;
		m_res.color_render_texture_spec.width = Window::GetWidth();
		m_res.color_render_texture_spec.height = Window::GetHeight();
		m_res.color_render_texture_spec.wrap_params = GL_CLAMP_TO_EDGE;

		m_res.p_scene_display_texture = std::make_unique<Texture2D>("Editor scene display");
		m_res.p_scene_display_texture->SetSpec(m_res.color_render_texture_spec);

		// Adding a resize event listener so the scene display texture scales with the window
		m_window_event_listener.OnEvent = [this](const Events::WindowEvent& t_event) {
			if (t_event.event_type == Events::Event::EventType::WINDOW_RESIZE) {
				auto spec = m_res.p_scene_display_texture->GetSpec();
				spec.width = t_event.new_window_size.x;
				spec.height = t_event.new_window_size.y;
				m_res.p_scene_display_texture->SetSpec(spec);

				UpdateSceneDisplayRect();
			}
			};


		Events::EventManager::RegisterListener(m_window_event_listener);

		Texture2DSpec picking_spec;
		picking_spec.format = GL_RGB_INTEGER;
		picking_spec.internal_format = GL_RGB32UI;
		picking_spec.storage_type = GL_UNSIGNED_INT;
		picking_spec.width = Window::GetWidth();
		picking_spec.height = Window::GetHeight();

		// Entity ID's are split into halves for storage in textures then recombined later as there is no format for 64 bit uints
		m_res.p_picking_fb = &Renderer::GetFramebufferLibrary().CreateFramebuffer("picking", true);
		m_res.p_picking_fb->AddRenderbuffer(Window::GetWidth(), Window::GetHeight());
		m_res.p_picking_fb->Add2DTexture("component_ids_split", GL_COLOR_ATTACHMENT0, picking_spec);

		SceneRenderer::SetActiveScene(&*SCENE);
		static auto s = &*SCENE;
		m_event_stack.SetContext(s, &m_state.selected_entity_ids);

		m_res.p_editor_pass_fb = &Renderer::GetFramebufferLibrary().CreateFramebuffer("editor_passes", true);
		m_res.p_editor_pass_fb->AddShared2DTexture("shared_depth", Renderer::GetFramebufferLibrary().GetFramebuffer("gbuffer").GetTexture<Texture2D>("shared_depth"), GL_DEPTH_ATTACHMENT);
		m_res.p_editor_pass_fb->AddShared2DTexture("Editor scene display", *m_res.p_scene_display_texture, GL_COLOR_ATTACHMENT0);
		m_res.p_editor_pass_fb->AddRenderbuffer(Window::GetWidth(), Window::GetHeight());

		std::string base_proj_dir = m_state.executable_directory + "\\projects\\base-project";
		if (!std::filesystem::exists(base_proj_dir)) {
			GenerateProject("base-project", false);
		}

		MakeProjectActive(m_start_filepath);

		UpdateSceneDisplayRect();

		ORNG_CORE_INFO("Editor layer initialized");
		EventManager::DispatchEvent(EditorEvent(EditorEventType::POST_INITIALIZATION));

		m_asset_manager_window.p_extern_scene = mp_scene_context->get();
		m_asset_manager_window.Init();
	}


	void EditorLayer::UpdateSceneDisplayRect() {
		if (m_state.fullscreen_scene_display)
			m_state.scene_display_rect = { ImVec2(Window::GetWidth(), Window::GetHeight() - m_res.toolbar_height) };
		else
			m_state.scene_display_rect = { ImVec2(Window::GetWidth() - (LEFT_WINDOW_WIDTH + RIGHT_WINDOW_WIDTH), Window::GetHeight() - m_asset_manager_window.window_height - m_res.toolbar_height) };

		mp_editor_camera->GetComponent<CameraComponent>()->aspect_ratio = m_state.scene_display_rect.x / m_state.scene_display_rect.y;
	}


	void EditorLayer::BeginPlayScene() {
		ORNG_TRACY_PROFILE;
		SceneSerializer::SerializeScene(*SCENE, m_state.temp_scene_serialization, true);
		m_state.simulate_mode_active = true;

		// Set to fullscreen so mouse coordinate and gui operations in scripts work correctly as they would in a runtime layer
		m_state.fullscreen_scene_display = true;
		SCENE->OnStart();

		UpdateSceneDisplayRect();

		EventManager::DispatchEvent(EditorEvent(EditorEventType::SCENE_START_SIMULATION));
	}

	void EditorLayer::EndPlayScene() {
		ORNG_TRACY_PROFILE;

		SCENE->m_time_elapsed = 0.0;

		// Reset mouse state as scripts may have modified it
		Input::SetCursorVisible(true);

		m_state.selected_entity_ids.clear();
		m_state.p_selected_joint = nullptr;

		auto* p_cam_transform = mp_editor_camera->GetComponent<TransformComponent>();
		glm::vec3 cam_pos = p_cam_transform->GetAbsPosition();
		glm::vec3 look_at_pos = cam_pos + p_cam_transform->forward;

		mp_editor_camera = nullptr;

		SCENE->ClearAllEntities();
		SceneSerializer::DeserializeScene(*SCENE, m_state.temp_scene_serialization, false);

		mp_editor_camera = std::make_unique<SceneEntity>(&*SCENE, SCENE->m_registry.create(), &SCENE->m_registry, SCENE->uuid());
		auto* p_transform = mp_editor_camera->AddComponent<TransformComponent>();
		p_transform->SetAbsolutePosition(cam_pos);
		p_transform->LookAt(look_at_pos);
		mp_editor_camera->AddComponent<CameraComponent>()->MakeActive();

		m_state.simulate_mode_active = false;
		m_state.fullscreen_scene_display = false;

		UpdateSceneDisplayRect();

		EventManager::DispatchEvent(EditorEvent(EditorEventType::SCENE_END_SIMULATION));
	}


	void EditorLayer::OnShutdown() {
		if (m_state.simulate_mode_active)
			EndPlayScene();

		mp_editor_camera = nullptr;
		SCENE->UnloadScene();
		m_asset_manager_window.OnShutdown();
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


		ImGui::GetStyle().Colors[ImGuiCol_Button] = m_res.lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = m_res.lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = m_res.lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_Border] = m_res.lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_Tab] = m_res.lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_TabHovered] = m_res.lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_TabActive] = m_res.orange_color;
		ImGui::GetStyle().Colors[ImGuiCol_Header] = m_res.lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_HeaderHovered] = m_res.lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_HeaderActive] = m_res.orange_color;
		ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = m_res.lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_FrameBgHovered] = m_res.lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_FrameBgActive] = m_res.lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_DockingEmptyBg] = m_res.lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = m_res.dark_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_Border] = m_res.lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_TitleBg] = m_res.lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive] = m_res.lighter_grey_color;
	}

	void EditorLayer::PollKeybinds() {
		if (!ImGui::GetIO().WantCaptureKeyboard && !Input::IsMouseDown(1)) {
			// Tab - Show/hide ui in simulation mode
			if (Input::IsKeyPressed(Key::Tab)) {
				m_state.render_ui_in_simulation = !m_state.render_ui_in_simulation;
			}

			if (m_state.selection_mode == SelectionMode::ENTITY) {
				// Ctrl+D - Duplicate entity
				if (Input::IsKeyDown(Key::LeftControl) && Input::IsKeyPressed(Key::D)) {
					auto vec = DuplicateEntitiesTracked(m_state.selected_entity_ids);
					std::vector<uint64_t> duplicate_ids;
					for (auto* p_ent : vec) {
						duplicate_ids.push_back(p_ent->uuid());
					}
					m_state.selected_entity_ids = duplicate_ids;
				}

				// Ctrl+x - Delete entity
				if (Input::IsKeyDown(Key::LeftControl) && Input::IsKeyPressed(Key::X)) {
					DeleteEntitiesTracked(m_state.selected_entity_ids);
				}
			}
			else if (m_state.selection_mode == SelectionMode::JOINT) {
				if (Input::IsKeyDown(Key::LeftControl) && Input::IsKeyPressed(Key::X)) {
					m_state.p_selected_joint->Break();
					m_state.p_selected_joint = nullptr;
				}
			}

			// Ctrl+z / Ctrl+shift+z undo/redo
			if (Input::IsKeyDown(Key::LeftControl) && Input::IsKeyPressed('z') && !m_state.simulate_mode_active) { // Undo/redo disabled in simulation mode to prevent overlap during (de)serialization switching back and forth
				if (Input::IsKeyDown(Key::Shift))
					m_event_stack.Redo();
				else
					m_event_stack.Undo();
			}

			// K - Make editor cam active
			if (Input::IsKeyDown(Key::K))
				mp_editor_camera->GetComponent<CameraComponent>()->MakeActive();

			// F - Focus on entity
			if (Input::IsKeyDown(Key::F) && !m_state.selected_entity_ids.empty()) {
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
			if (m_state.simulate_mode_active && Input::IsKeyDown(Key::Escape)) {
				EndPlayScene();
			}
		}

		// Drag state tracking
		static bool dragging = false;
		if (Input::IsMouseClicked(0)) {
			m_state.mouse_drag_data.start = Input::GetMousePos();
			if (!m_state.fullscreen_scene_display) m_state.mouse_drag_data.start = ConvertFullscreenMouseToDisplayMouse(m_state.mouse_drag_data.start);
		}

		if (dragging) {
			m_state.mouse_drag_data.end = Input::GetMousePos();
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

		if (m_state.simulate_mode_active && !m_state.simulate_mode_paused)
			SCENE->Update(FrameTiming::GetTimeStep());
		else {
			SCENE->m_mesh_component_manager.OnUpdate(); // This still needs to update so meshes are rendered correctly in the editor
			SCENE->m_particle_system.OnUpdate(); // Continue simulating particles for visual feedback
			SCENE->m_audio_system.OnUpdate(); // For accurate audio playback
			//SCENE->terrain.UpdateTerrainQuadtree(SCENE->m_camera_system.GetActiveCamera()->GetEntity()->GetComponent<TransformComponent>()->GetPosition()); // Needed for terrain LOD updates
		}
	}

	void EditorLayer::MultiSelectDisplay() {
		m_state.selected_entity_ids.clear();

		glm::vec2 min = { glm::min(m_state.mouse_drag_data.start.x,  m_state.mouse_drag_data.end.x), glm::min(Window::GetHeight() - m_state.mouse_drag_data.start.y,  Window::GetHeight() - m_state.mouse_drag_data.end.y) };
		glm::vec2 max = { glm::max(m_state.mouse_drag_data.start.x,  m_state.mouse_drag_data.end.x), glm::max(Window::GetHeight() - m_state.mouse_drag_data.start.y, Window::GetHeight() - m_state.mouse_drag_data.end.y) };
		glm::vec2 n = glm::vec2(m_state.scene_display_rect.x, m_state.scene_display_rect.y);


		auto* p_cam = SCENE->GetActiveCamera();
		auto* p_transform = p_cam->GetEntity()->GetComponent<TransformComponent>();
		auto pos = p_transform->GetAbsPosition();

		glm::vec3 min_dir = ExtraMath::ScreenCoordsToRayDir(p_cam->GetProjectionMatrix(), min, pos, p_transform->forward, p_transform->up, Window::GetWidth(), Window::GetHeight());
		glm::vec3 max_dir = ExtraMath::ScreenCoordsToRayDir(p_cam->GetProjectionMatrix(), max, pos, p_transform->forward, p_transform->up, Window::GetWidth(), Window::GetHeight());

		glm::vec3 near_min = pos + min_dir * p_cam->zNear;

		glm::vec3 far_min = pos + min_dir * p_cam->zFar;

		glm::vec3 near_max = pos + max_dir * p_cam->zNear;

		glm::vec3 far_max = pos + max_dir * p_cam->zFar;


		float ratio = (float)glm::abs(m_state.mouse_drag_data.start.x - m_state.mouse_drag_data.end.x) / glm::abs(m_state.mouse_drag_data.start.y - m_state.mouse_drag_data.end.y);
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
						SelectEntity(SCENE->GetEntity(entity)->uuid());
					else if (m_state.general_settings.selection_settings.select_mesh_objects && p_entity->HasComponent<MeshComponent>())
						SelectEntity(SCENE->GetEntity(entity)->uuid());
					else if (m_state.general_settings.selection_settings.select_physics_objects && p_entity->HasComponent<PhysicsComponent>())
						SelectEntity(SCENE->GetEntity(entity)->uuid());
				}
				else {
					SelectEntity(SCENE->GetEntity(entity)->uuid());
				}
			}
		}
	}



	void EditorLayer::UpdateEditorCam() {
		static float cam_speed = 0.01f;

		auto* p_cam = mp_editor_camera->GetComponent<CameraComponent>();
		auto* p_transform = mp_editor_camera->GetComponent<TransformComponent>();
		p_cam->aspect_ratio = m_state.scene_display_rect.x / m_state.scene_display_rect.y;
		// Camera movement
		if (ImGui::IsMouseDown(1)) {
			glm::vec3 pos = p_transform->GetAbsPosition();
			glm::vec3 movement_vec{ 0.0, 0.0, 0.0 };
			float time_elapsed = FrameTiming::GetTimeStep();
			movement_vec += p_transform->right * (float)Input::IsKeyDown(Key::D) * time_elapsed * cam_speed;
			movement_vec -= p_transform->right * (float)Input::IsKeyDown(Key::A) * time_elapsed * cam_speed;
			movement_vec += p_transform->forward * (float)Input::IsKeyDown(Key::W) * time_elapsed * cam_speed;
			movement_vec -= p_transform->forward * (float)Input::IsKeyDown(Key::S) * time_elapsed * cam_speed;
			movement_vec += glm::vec3(0, 1, 0) * (float)Input::IsKeyDown(Key::E) * time_elapsed * cam_speed;
			movement_vec -= glm::vec3(0, 1, 0) * (float)Input::IsKeyDown(Key::Q) * time_elapsed * cam_speed;

			if (Input::IsKeyDown(Key::Space))
				movement_vec *= 10.0;

			if (Input::IsKeyDown(Key::Shift))
				movement_vec *= 100.0;

			if (Input::IsKeyDown(Key::LeftControl))
				movement_vec *= 0.1;

			p_transform->SetAbsolutePosition(pos + movement_vec);
		}

		// Camera rotation
		static glm::vec2 last_mouse_pos;
		if (ImGui::IsMouseClicked(1))
			last_mouse_pos = Input::GetMousePos();

		if (ImGui::IsMouseDown(1)) {
			float rotation_speed = 0.005f;
			glm::vec2 mouse_coords = Input::GetMousePos();
			glm::vec2 mouse_delta = -glm::vec2(mouse_coords.x - last_mouse_pos.x, mouse_coords.y - last_mouse_pos.y);

			glm::vec3 rot_x = glm::rotate(mouse_delta.x * rotation_speed, glm::vec3(0.0, 1.0, 0.0)) * glm::vec4(p_transform->forward, 0);
			glm::fvec3 rot_y = glm::rotate(mouse_delta.y * rotation_speed, p_transform->right) * glm::vec4(rot_x, 0);


			if (rot_y.y <= 0.997f && rot_y.y >= -0.997f)
				p_transform->LookAt(p_transform->GetAbsPosition() + glm::normalize(rot_y));
			else
				p_transform->LookAt(p_transform->GetAbsPosition() + glm::normalize(rot_x));


			Window::SetCursorPos(last_mouse_pos.x, last_mouse_pos.y);
		}

		p_cam->UpdateFrustum();

		// Update camera pos variable for lua console
		auto pos = mp_editor_camera->GetComponent<TransformComponent>()->GetPosition();
		m_lua_cli.GetLua().script(std::format("pos = vec3.new({}, {}, {})", pos.x, pos.y, pos.z));
	}





	void EditorLayer::RenderSceneDisplayPanel() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::SetNextWindowPos(AddImVec2(ImGui::GetMainViewport()->Pos, ImVec2(LEFT_WINDOW_WIDTH, m_res.toolbar_height)));
		ImGui::SetNextWindowSize(m_state.scene_display_rect);


		if (ImGui::Begin("Scene display overlay", (bool*)0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | (ImGui::IsMouseDragging(0) ? 0 : ImGuiWindowFlags_NoInputs) | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground)) {
			ImVec2 prev_curs_pos = ImGui::GetCursorPos();
			ImGui::Image((ImTextureID)m_res.p_scene_display_texture->GetTextureHandle(), ImVec2(m_state.scene_display_rect.x, m_state.scene_display_rect.y), ImVec2(0, 1), ImVec2(1, 0));
			ImGui::SetCursorPos(prev_curs_pos);
			ImGui::Dummy(ImVec2(0, 5));
			ImGui::Dummy(ImVec2(5, 0));
			ImGui::SameLine();
			ImGui::InvisibleButton("##drag-drop-scene-target", ImVec2(m_state.scene_display_rect.x - 20, m_state.scene_display_rect.y - 20));

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("ENTITY")) {
					/* Functionality handled externally */
				}
				else if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("MESH")) {
					if (p_payload->DataSize == sizeof(MeshAsset*)) {
						auto& ent = CreateEntityTracked("Mesh");
						ent.AddComponent<MeshComponent>(*static_cast<MeshAsset**>(p_payload->Data));
						auto* p_cam_transform = mp_editor_camera->GetComponent<TransformComponent>();
						auto& mesh_aabb = ent.GetComponent<MeshComponent>()->GetMeshData()->m_aabb;
						ent.GetComponent<TransformComponent>()->SetAbsolutePosition(p_cam_transform->GetAbsPosition() + p_cam_transform->forward * (glm::max(glm::max(mesh_aabb.max.x, mesh_aabb.max.y), mesh_aabb.max.z) + 5.f));
						SelectEntity(ent.GetUUID());
					}
				}
				else if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("PREFAB")) {
					if (p_payload->DataSize == sizeof(Prefab*)) {
						Prefab* prefab_data = (*static_cast<Prefab**>(p_payload->Data));
						auto& ent = m_state.simulate_mode_active ? SCENE->InstantiatePrefabCallScript(*prefab_data) : SCENE->InstantiatePrefab(*prefab_data);
						auto* p_cam_transform = mp_editor_camera->GetComponent<TransformComponent>();
						glm::vec3 pos;
						if (auto* p_mesh = ent.GetComponent<MeshComponent>()) {
							auto& mesh_aabb = p_mesh->GetMeshData()->m_aabb;
							pos = p_cam_transform->GetAbsPosition() + p_cam_transform->forward * (glm::max(glm::max(mesh_aabb.max.x, mesh_aabb.max.y), mesh_aabb.max.z) + 5.f);
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
		if (m_state.simulate_mode_active && !m_state.render_ui_in_simulation) {
			RenderToolbar();
			return;
		}

		if (!m_state.simulate_mode_active)
			RenderSceneDisplayPanel();


		RenderToolbar();

		m_lua_cli.render_pos = { LEFT_WINDOW_WIDTH, m_res.toolbar_height };
		m_lua_cli.size = { m_state.scene_display_rect.x, 250 };
		m_lua_cli.OnImGuiRender(!Input::IsMouseDown(1));

		m_asset_manager_window.OnRenderUI();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 5);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
		ImGui::SetNextWindowPos(AddImVec2(ImGui::GetMainViewport()->Pos, ImVec2(0, m_res.toolbar_height)));
		ImGui::SetNextWindowSize(ImVec2(LEFT_WINDOW_WIDTH, Window::GetHeight() - m_res.toolbar_height - m_asset_manager_window.window_height));
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);

		ImGui::Begin("##left window", (bool*)0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar);
		ImGui::End();

		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		RenderSceneGraph();

		ImGui::SetNextWindowPos(AddImVec2(ImGui::GetMainViewport()->Pos, ImVec2(Window::GetWidth() - RIGHT_WINDOW_WIDTH, m_res.toolbar_height)));
		ImGui::SetNextWindowSize(ImVec2(RIGHT_WINDOW_WIDTH, Window::GetHeight() - m_res.toolbar_height));
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::Begin("##right window", (bool*)0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus)) {
			DisplayEntityEditor();
		}
		ImGui::End();

		RenderGeneralSettingsMenu();

		if (m_state.general_settings.editor_window_settings.display_joint_maker)
			RenderJointMaker();

		ImGui::PopStyleVar(); // window border size
		ImGui::PopStyleVar(); // window padding
	}




	void EditorLayer::RenderDisplayWindow() {
		Renderer::GetFramebufferLibrary().UnbindAllFramebuffers();
		GL_StateManager::DefaultClearBits();


		if (ImGui::IsMouseClicked(0) && !ImGui::GetIO().WantCaptureMouse) {
			DoPickingPass();
		}


		SceneRenderer::SceneRenderingSettings settings;
		SceneRenderer::SetActiveScene(&**mp_scene_context);
		settings.p_output_tex = &*m_res.p_scene_display_texture;
		settings.render_meshes = m_state.general_settings.debug_render_settings.render_meshes;
		settings.voxel_mip_layer = m_state.general_settings.debug_render_settings.voxel_mip;
		settings.voxel_render_face = m_state.general_settings.debug_render_settings.voxel_render_face;

		if (m_state.general_settings.debug_render_settings.render_wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		SceneRenderer::RenderScene(settings);

		m_asset_manager_window.OnMainRender();
		// Mouse drag selection quad
		if (ImGui::IsMouseDragging(0) && !ImGui::GetIO().WantCaptureMouse) {
			m_res.p_quad_col_shader->ActivateProgram();
			m_res.p_quad_col_shader->SetUniform("u_color", glm::vec4(0, 0, 1, 0.1));
			glm::vec2 w = { Window::GetWidth(), Window::GetHeight() };
			Renderer::DrawScaledQuad((glm::vec2(m_state.mouse_drag_data.start.x, Window::GetHeight() - m_state.mouse_drag_data.start.y) / w) * 2.f - 1.f, (glm::vec2(m_state.mouse_drag_data.end.x, Window::GetHeight() - m_state.mouse_drag_data.end.y) / w) * 2.f - 1.f);
		}

		m_res.p_editor_pass_fb->Bind();
		RenderGrid();
		DoSelectedEntityHighlightPass();
		if (m_state.general_settings.debug_render_settings.render_physx_debug)
			RenderPhysxDebug();

		Renderer::GetFramebufferLibrary().UnbindAllFramebuffers();
		GL_StateManager::DefaultClearBits();

		if (m_state.simulate_mode_active) {
			Renderer::GetShaderLibrary().GetQuadShader().ActivateProgram();
			GL_StateManager::BindTexture(GL_TEXTURE_2D, m_res.p_scene_display_texture->GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR);
			Renderer::DrawQuad();
		}


	}




	void EditorLayer::RenderGeneralSettingsMenu() {
		if (Input::IsKeyDown(Key::LeftControl) && Input::IsKeyPressed('j')) {
			m_state.general_settings.editor_window_settings.display_settings_window = !m_state.general_settings.editor_window_settings.display_settings_window;
		}

		if (!m_state.general_settings.editor_window_settings.display_settings_window)
			return;

		if (ImGui::Begin("Settings")) {
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
						m_state.general_settings.debug_render_settings.voxel_render_face = (VoxelRenderFace)(1 << i);
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
		ImGui::SetNextWindowSize(ImVec2(Window::GetWidth(), m_res.toolbar_height));

		if (ImGui::Begin("##Toolbar", (bool*)0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNavInputs)) {
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
				for (int i = 0; i < file_options.size(); i++) {
					if (ImGui::Selectable(file_options[i].c_str()))
						selected_component = i + 1;
				}
				ImGui::EndPopup();
			}

			switch (selected_component) {
			case 1:
				RenderProjectGenerator(selected_component);
				// Allow selected_component to keep its value so this can continue rendering until another option is picked
				break;
			case 2: {
				//setting up file explorer callbacks
				wchar_t valid_extensions[MAX_PATH] = L"Project Files: *.yml\0*.yml\0";
				std::function<void(std::string)> success_callback = [this](std::string filepath) {
					MakeProjectActive(filepath.substr(0, filepath.find_last_of('\\')));
					};

				ExtraUI::ShowFileExplorer(m_state.executable_directory + "/projects", valid_extensions, success_callback);
				selected_component = 0;
				break;
			}
			case 3: {
				std::string scene_filepath{ "scene.yml" };
				std::string uuid_filepath{ "./res/scripts/includes/uuids.h" };
				AssetManager::SerializeAssets();
				SceneSerializer::SerializeScene(*SCENE, scene_filepath);
				SceneSerializer::SerializeSceneUUIDs(*SCENE, uuid_filepath);
				selected_component = 0;
				break;
			}
			case 4: {
				BuildGameFromActiveProject();
				selected_component = 0;
				break;
			}
			}


			ImGui::SameLine();
			if (m_state.simulate_mode_active && ImGui::Button(ICON_FA_CIRCLE))
				EndPlayScene();
			else if (!m_state.simulate_mode_active && ImGui::Button(ICON_FA_PLAY))
				BeginPlayScene();

			ImGui::SameLine();
			if (m_state.simulate_mode_active && ((!m_state.simulate_mode_paused && ImGui::Button((ICON_FA_PAUSE)) || (m_state.simulate_mode_paused && ImGui::Button((ICON_FA_ANGLES_UP)))))) {
				m_state.simulate_mode_paused = !m_state.simulate_mode_paused;
			}


			ImGui::SameLine();

			if (ImGui::Button("Reload shaders")) {
				Renderer::GetShaderLibrary().ReloadShaders();
			}
			ImGui::SameLine();
			
			// Transform gizmos
			ImGui::Dummy({ 100, 0 });
			ImGui::SameLine();
			if (ExtraUI::SwitchButton(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT, m_state.current_gizmo_operation == ImGuizmo::TRANSLATE, m_res.lighter_grey_color, m_res.blue_col))
				m_state.current_gizmo_operation = ImGuizmo::TRANSLATE;
			ImGui::SameLine();

			if (ExtraUI::SwitchButton(ICON_FA_COMPRESS, m_state.current_gizmo_operation == ImGuizmo::SCALE, m_res.lighter_grey_color, m_res.blue_col))
				m_state.current_gizmo_operation = ImGuizmo::SCALE;
			ImGui::SameLine();

			if (ExtraUI::SwitchButton(ICON_FA_ROTATE_RIGHT, m_state.current_gizmo_operation == ImGuizmo::ROTATE, m_res.lighter_grey_color, m_res.blue_col))
				m_state.current_gizmo_operation = ImGuizmo::ROTATE;
			ImGui::SameLine();

			if (!ImGui::GetIO().WantCaptureMouse) {
				if (Input::IsKeyPressed(GLFW_KEY_1))
					m_state.current_gizmo_operation = ImGuizmo::TRANSLATE;
				else if (Input::IsKeyPressed(GLFW_KEY_2))
					m_state.current_gizmo_operation = ImGuizmo::SCALE;
				else if (Input::IsKeyPressed(GLFW_KEY_3))
					m_state.current_gizmo_operation = ImGuizmo::ROTATE;
			}


			if (ImGui::Button("Transform mode")) {
				ImGui::OpenPopup("##transform mode");
			}

			if (ImGui::BeginPopup("##transform mode")) {
				if (ImGui::Selectable("Entities", m_state.selection_mode == SelectionMode::ENTITY))
					m_state.selection_mode = SelectionMode::ENTITY;
				if (ImGui::Selectable("Joints", m_state.selection_mode == SelectionMode::JOINT))
					m_state.selection_mode = SelectionMode::JOINT;

				ImGui::EndPopup();
			}

			if (!ImGui::GetIO().WantCaptureMouse) {
				if (Input::IsKeyPressed(GLFW_KEY_4))
					m_state.selection_mode = SelectionMode::ENTITY;
				else if (Input::IsKeyPressed(GLFW_KEY_5))
					m_state.selection_mode = SelectionMode::JOINT;
			}

			// Additional windows
			ImGui::SameLine();
			ImGui::Dummy({ 100, 0 });
			ImGui::SameLine();
			if (ExtraUI::SwitchButton(ICON_FA_DIAGRAM_PROJECT, m_state.general_settings.editor_window_settings.display_joint_maker, m_res.lighter_grey_color, m_res.blue_col)) {
				m_state.general_settings.editor_window_settings.display_joint_maker = !m_state.general_settings.editor_window_settings.display_joint_maker;
			}
			ExtraUI::TooltipOnHover("Joint maker");
			ImGui::SameLine();

			if (ExtraUI::SwitchButton(ICON_FA_P, m_state.general_settings.debug_render_settings.render_physx_debug, m_res.lighter_grey_color, m_res.blue_col)) {
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


	void EditorLayer::BuildGameFromActiveProject() {
		// Delete old build
		TryFileDelete("build");

		Create_Directory("build");
		FileCopy(m_state.current_project_directory + "/scene.yml", "build/scene.yml");
		FileCopy(m_state.current_project_directory + "/res", "build/res", true);
		FileCopy(GetApplicationExecutableDirectory() + "/res/shaders", "build/res/shaders", true);
		FileCopy(GetApplicationExecutableDirectory() + "/../ORNG-Runtime/ORNG_RUNTIME.exe", "build/ORNG_RUNTIME.exe");

		std::vector<std::string> dlls = {
			"fmod.dll", "PhysX_64.dll", "PhysXCommon_64.dll", "PhysXCooking_64.dll", "PhysXFoundation_64.dll"
		};

		for (const auto& path : dlls) {
			FileCopy(GetApplicationExecutableDirectory() + "/../ORNG-Runtime/" + path, "build/" + path);
		}
	}

	void EditorLayer::RenderProjectGenerator(int& selected_component_from_popup) {
		ImGui::SetNextWindowSize(ImVec2(500, 200));
		ImGui::SetNextWindowPos(ImVec2(Window::GetWidth() / 2 - 250, Window::GetHeight() / 2 - 100));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10);
		static std::string project_dir;

		if (ImGui::Begin("##project gen", (bool*)0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
			if (ImGui::IsMouseDoubleClicked(1)) // close window
				selected_component_from_popup = 0;

			ImGui::PushFont(m_res.p_xl_font);
			ImGui::SeparatorText("Generate project");
			ImGui::PopFont();
			ImGui::Text("Path");
			ImGui::InputText("#pdir", &project_dir);
			
			static std::string err_msg = "";
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
			ImGui::TextWrapped(err_msg.c_str());
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
	void RefreshScriptIncludes() {
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/scene/SceneEntity.h", "./res/scripts/includes/SceneEntity.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/scene/Scene.h", "./res/scripts/includes/Scene.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/scripting/ScriptAPI.h", "./res/scripts/includes/ScriptAPI.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/components/Component.h", "./res/scripts/includes/Component.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/components/MeshComponent.h", "./res/scripts/includes/MeshComponent.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/components/ScriptComponent.h", "./res/scripts/includes/ScriptComponent.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/components/Lights.h", "./res/scripts/includes/Lights.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/extern/glm/glm/", "./res/scripts/includes/glm/", true);
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/components/TransformComponent.h", "./res/scripts/includes/TransformComponent.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/components/PhysicsComponent.h", "./res/scripts/includes/PhysicsComponent.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/components/CameraComponent.h", "./res/scripts/includes/CameraComponent.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/extern/entt/EnttSingleInclude.h", "./res/scripts/includes/EnttSingleInclude.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/components/AudioComponent.h", "./res/scripts/includes/AudioComponent.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/scripting/ScriptShared.h", "./res/scripts/includes/ScriptShared.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/scripting/ScriptInstancer.h", "./res/scripts/includes/ScriptInstancer.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/components/VehicleComponent.h", "./res/scripts/includes/VehicleComponent.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/components/ParticleEmitterComponent.h", "./res/scripts/includes/ParticleEmitterComponent.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/scripting/ScriptAPIImpl.h", "./res/scripts/includes/ScriptAPIImpl.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/scripting/SI.h", "./res/scripts/includes/SI.h");

		if (!FileExists(ORNG_CORE_LIB_DIR "../vcpkg_installed/x64-windows/include")) {
			ORNG_CORE_ERROR("Physx include dir not found, scripts referencing the physx API will not compile");
		}
		else {
			FileCopy(ORNG_CORE_LIB_DIR "../vcpkg_installed/x64-windows/include/physx/", "./res/scripts/includes/physx/", true);
		}

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
		// Create base scene for project to use
		std::ofstream s{ project_path + "/scene.yml" };
		s << ORNG_BASE_SCENE_YAML;
		s.close();


		std::filesystem::create_directory(project_path + "/res");
		std::filesystem::create_directory(project_path + "/res/meshes");
		std::filesystem::create_directory(project_path + "/res/textures");
		std::filesystem::create_directory(project_path + "/res/scripts");
		std::filesystem::create_directory(project_path + "/res/scripts/includes");
		std::filesystem::create_directory(project_path + "/res/scripts/bin");
		std::filesystem::create_directory(project_path + "/res/scripts/bin/release");
		std::filesystem::create_directory(project_path + "/res/scripts/bin/debug");
		std::filesystem::create_directory(project_path + "/res/shaders");
		std::filesystem::create_directory(project_path + "/res/audio");
		std::filesystem::create_directory(project_path + "/res/prefabs");
		std::filesystem::create_directory(project_path + "/res/materials");
		std::filesystem::create_directory(project_path + "/res/physx-materials");

		return true;
	}




	bool EditorLayer::ValidateProjectDir(const std::string& dir_path) {
		try {
			if (!std::filesystem::exists(dir_path + "/scene.yml")) {
				std::ofstream s{ dir_path + "/scene.yml" };
				s << ORNG_BASE_SCENE_YAML;
				s.close();
			}

			// If any of these are missing, will be recreated
			Create_Directory(dir_path + "/res/");
			Create_Directory(dir_path + "/res/meshes");
			Create_Directory(dir_path + "/res/textures");
			Create_Directory(dir_path + "/res/shaders");
			Create_Directory(dir_path + "/res/scripts");
			Create_Directory(dir_path + "/res/scripts/bin");
			Create_Directory(dir_path + "/res/scripts/bin/release");
			Create_Directory(dir_path + "/res/scripts/bin/debug");
			Create_Directory(dir_path + "/res/scripts/includes");
			Create_Directory(dir_path + "/res/prefabs");
			Create_Directory(dir_path + "/res/materials");
			Create_Directory(dir_path + "/res/audio");
			Create_Directory(dir_path + "/res/physx-materials");


			std::ifstream stream(dir_path + "/scene.yml");
			std::stringstream str_stream;
			str_stream << stream.rdbuf();
			stream.close();

			YAML::Node data = YAML::Load(str_stream.str());
			if (!data.IsDefined() || data.IsNull() || !data["Scene"]) {
				std::filesystem::copy_file(dir_path + "/scene.yml", dir_path + "/sceneCORRUPTED.yml");
				ORNG_CORE_ERROR("scene.yml file is corrupted, replacing with default template");
				std::ofstream s{ dir_path + "/scene.yml" };
				s << ORNG_BASE_SCENE_YAML;
				s.close();
			}
		}
		catch (const std::exception& e) {
			ORNG_CORE_ERROR("Error validating project '{0}', : '{1}'", dir_path, e.what());
			return false;
		}

		return true;
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
			AssetManager::LoadAssetsFromProjectPath(m_state.current_project_directory, false);
			SCENE->LoadScene(m_state.current_project_directory + "\\scene.yml");
			SceneSerializer::DeserializeScene(*SCENE, m_state.current_project_directory + "\\scene.yml", true);

			mp_editor_camera = std::make_unique<SceneEntity>(&*SCENE, SCENE->m_registry.create(), &SCENE->m_registry, SCENE->uuid());
			auto* p_transform = mp_editor_camera->AddComponent<TransformComponent>();
			p_transform->SetAbsolutePosition(cam_pos);
			mp_editor_camera->AddComponent<CameraComponent>()->MakeActive();

			EventManager::DispatchEvent(EditorEvent(EditorEventType::POST_SCENE_LOAD));
		}
		else {
			ORNG_CORE_ERROR("Project folder path invalid");
			return false;
		}
		return true;
	}



	void EditorLayer::RenderCreationWidget(SceneEntity* p_entity, bool trigger) {
		const char* names[13] = { "Pointlight", "Spotlight", "Mesh", "Camera", "Physics", "Script", "Audio", "Vehicle", "Particle emitter", "Billboard", "Particle buffer", "Character controller", "Joint"};

		if (trigger)
			ImGui::OpenPopup("my_select_popup");

		int selected_component = -1;
		if (ImGui::BeginPopup("my_select_popup"))
		{
			ImGui::SeparatorText(p_entity ? "Add component" : "Create entity");
			for (int i = 0; i < IM_ARRAYSIZE(names); i++)
				if (ImGui::Selectable(names[i]))
					selected_component = i;
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
		case 7:
			entity->AddComponent<VehicleComponent>();
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
		}
	}



	glm::vec2 EditorLayer::ConvertFullscreenMouseToDisplayMouse(glm::vec2 mouse_coords) {
		// Transform mouse coordinates to full window space for the proper texture coordinates
		mouse_coords.x -= LEFT_WINDOW_WIDTH;
		mouse_coords.x *= (Window::GetWidth() / ((float)Window::GetWidth() - (RIGHT_WINDOW_WIDTH + LEFT_WINDOW_WIDTH)));
		mouse_coords.y -= m_res.toolbar_height;
		mouse_coords.y *= (float)Window::GetHeight() / ((float)Window::GetHeight() - m_asset_manager_window.window_height - m_res.toolbar_height);
		return mouse_coords;
	}


	void EditorLayer::DoPickingPass() {
		m_res.p_picking_fb->Bind();
		m_res.p_picking_shader->ActivateProgram();

		GL_StateManager::ClearDepthBits();
		GL_StateManager::ClearBitsUnsignedInt(UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX);

		// Mesh picking
		auto view = SCENE->m_registry.view<MeshComponent>();
		for (auto [entity, mesh] : view.each()) {
			//Split uint64 into two uint32's for texture storage
			uint64_t full_id = mesh.GetEntityUUID();
			glm::uvec3 id_vec{ (uint32_t)(full_id >> 32), (uint32_t)(full_id), UINT_MAX };

			m_res.p_picking_shader->SetUniform("comp_id", id_vec);
			m_res.p_picking_shader->SetUniform("transform", mesh.GetEntity()->GetComponent<TransformComponent>()->GetMatrix());

			Renderer::DrawMeshInstanced(mesh.GetMeshData(), 1);
		}

		// Joint picking
		m_state.p_selected_joint = nullptr;
		if (m_state.general_settings.debug_render_settings.render_physx_debug) {
			for (auto [entity, joint] : SCENE->m_registry.view<JointComponent>().each()) {
				for (auto it = joint.attachments.begin(); it != joint.attachments.end(); it++) {
					if (entity != it->first->GetA0()->GetEnttHandle()) // Ensure joints are only rendered by their owning entities to avoid rendering twice
						continue;

					auto* p_joint = it->first;

					//Split uint64 into two uint32's for texture storage
					uint64_t full_id = joint.GetEntityUUID();
					glm::uvec3 id_vec{ (uint32_t)(full_id >> 32), (uint32_t)(full_id), (uint32_t)std::distance(joint.attachments.begin(), it)};

					auto* p_transform = p_joint->GetA0()->GetComponent<TransformComponent>();
					auto pos = glm::quat(glm::radians(p_transform->GetAbsOrientation())) * p_joint->poses[0] + p_transform->GetAbsPosition();

					m_res.p_picking_shader->SetUniform("comp_id", id_vec);
					m_res.p_picking_shader->SetUniform("transform", glm::translate(pos) * glm::scale(glm::vec3(0.1)));

					Renderer::DrawSphere();
				}
			}
		}

		glm::vec2 mouse_coords = glm::min(
			glm::max(glm::vec2(Input::GetMousePos()), glm::vec2(1, 1)), 
			glm::vec2(Window::GetWidth() - 1, Window::GetHeight() - 1)
		);

		if (!m_state.fullscreen_scene_display) {
			mouse_coords = ConvertFullscreenMouseToDisplayMouse(mouse_coords);
		}

		uint32_t* pixels = new uint32_t[3];
		glReadPixels(mouse_coords.x, Window::GetHeight() - mouse_coords.y, 1, 1, GL_RGB_INTEGER, GL_UNSIGNED_INT, pixels);
		uint64_t current_entity_id = ((uint64_t)pixels[0] << 32) | pixels[1];

		if (!Input::IsKeyDown(GLFW_KEY_LEFT_CONTROL))
			m_state.selected_entity_ids.clear();

		if (pixels[2] != UINT_MAX) { // Joint selected
			SelectEntity(current_entity_id);
			auto* p_ent = SCENE->GetEntity(current_entity_id);

			if (p_ent) {
				if (auto* p_joint = p_ent->GetComponent<JointComponent>()) {
					auto it  = p_ent->GetComponent<JointComponent>()->attachments.begin();
					std::advance(it, pixels[2]);
					m_state.p_selected_joint = it->first;
				}
			}
		}
		else { // Mesh selected
			SelectEntity(current_entity_id);
		}

		delete[] pixels;

	}




	void EditorLayer::DoSelectedEntityHighlightPass() {
		m_res.p_editor_pass_fb->Bind();
		glEnable(GL_STENCIL_TEST);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		glClearStencil(0);
		glClear(GL_STENCIL_BUFFER_BIT);

		glStencilFunc(GL_ALWAYS, 1, 0xFF);

		glDisable(GL_DEPTH_TEST);
		m_res.p_highlight_shader->ActivateProgram();
		m_res.p_highlight_shader->SetUniform("u_color", glm::vec4(0.0, 1, 0, 0));

		for (auto id : m_state.selected_entity_ids) {
			auto* current_entity = SCENE->GetEntity(id);

			if (!current_entity || !current_entity->HasComponent<MeshComponent>())
				continue;

			MeshComponent* meshc = current_entity->GetComponent<MeshComponent>();

			m_res.p_highlight_shader->SetUniform("u_scale", 1.f);
			m_res.p_highlight_shader->SetUniform("transform", meshc->GetEntity()->GetComponent<TransformComponent>()->GetMatrix());
			Renderer::DrawMeshInstanced(meshc->GetMeshData(), 1);
		}

		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);


		for (auto id : m_state.selected_entity_ids) {
			auto* current_entity = SCENE->GetEntity(id);
			m_res.p_highlight_shader->SetUniform("u_color", glm::vec4(1.0, 0.2, 0, 1));

			if (!current_entity || !current_entity->HasComponent<MeshComponent>())
				continue;

			MeshComponent* meshc = current_entity->GetComponent<MeshComponent>();

			m_res.p_highlight_shader->SetUniform("u_scale", 1.025f);

			m_res.p_highlight_shader->SetUniform("transform", meshc->GetEntity()->GetComponent<TransformComponent>()->GetMatrix());
			Renderer::DrawMeshInstanced(meshc->GetMeshData(), 1);
		}

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);

	}


	void EditorLayer::RenderGrid() {
		m_res.grid_mesh->CheckBoundary(mp_editor_camera->GetComponent<TransformComponent>()->GetPosition());
		GL_StateManager::BindSSBO(m_res.grid_mesh->m_transform_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::TRANSFORMS);

		m_res.p_grid_shader->ActivateProgram();
		Renderer::DrawVAO_ArraysInstanced(GL_LINES, m_res.grid_mesh->m_vao, ceil(m_res.grid_mesh->grid_width / m_res.grid_mesh->grid_step) * 2);
	}


	void EditorLayer::UpdateLuaEntityArray() {
		std::string reset_script = R"(
			entity_array = 1
			entity_array = {}
		)";

		for (int i = 0; i < SCENE->m_entities.size(); i++) {
			auto* p_ent = SCENE->m_entities[i];
			auto* p_transform = p_ent->GetComponent<TransformComponent>();
			auto* p_relation_comp = p_ent->GetComponent<RelationshipComponent>();

			auto pos = p_transform->GetPosition();
			auto scale = p_transform->GetScale();
			auto rot = p_transform->GetOrientation();

			std::string entity_push_script = std::format("entity_array[{0}] = entity.new(\"{1}\", {2}, vec3.new({3}, {4}, {5}),  vec3.new({6}, {7}, {8}), vec3.new({9}, {10}, {11}), {12})", i+1, p_ent->name, (unsigned)p_ent->m_entt_handle, pos.x, pos.y, pos.z, scale.x, scale.y, scale.z, rot.x, rot.y, rot.z, (unsigned)p_relation_comp->parent);

			m_lua_cli.GetLua().script(entity_push_script);
		}


	}


	void EditorLayer::InitLua() {

		m_lua_cli.Init();
		m_lua_cli.input_callbacks.push_back([this] {
			UpdateLuaEntityArray();
			});

		m_lua_cli.GetLua().set_function("ORNG_select_entity", [this](unsigned handle) {
			auto* p_ent = SCENE->GetEntity(entt::entity(handle));
			if (p_ent)
				m_state.selected_entity_ids.push_back(p_ent->GetUUID());
			});

		m_lua_cli.GetLua().set_function("get_entity", [this](unsigned handle) -> LuaEntity {
			auto* p_ent = SCENE->GetEntity(entt::entity(handle));
			if (!p_ent)
				return LuaEntity{ "NULL", (unsigned)entt::null, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, (unsigned)entt::null };

			auto* p_transform = p_ent->GetComponent<TransformComponent>();
			auto* p_relation_comp = p_ent->GetComponent<RelationshipComponent>();

			return LuaEntity{ p_ent->name, (unsigned)p_ent->GetEnttHandle(), p_transform->GetPosition(), p_transform->GetScale(), p_transform->GetOrientation(), (unsigned)p_relation_comp->parent};

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
			TRANSFORM_LUA_SKELETON(p_transform->SetPosition(p_transform->GetPosition() + v));
			};

		std::function<void(glm::vec3)> p_scale_func = [this](glm::vec3 v) {
			TRANSFORM_LUA_SKELETON(p_transform->SetScale(p_transform->GetScale() * v));
			};

		std::function<void(glm::vec3, float)> p_rot_func = [this](glm::vec3 axis, float angle_degrees) {
			TRANSFORM_LUA_SKELETON(p_transform->SetOrientation(glm::degrees(glm::eulerAngles(glm::angleAxis(glm::radians(angle_degrees), axis) * glm::quat(glm::radians(p_transform->GetOrientation()))))));
			};

		std::function<void(glm::vec3)> p_move_to_func = [this](glm::vec3 pos) {
			TRANSFORM_LUA_SKELETON(p_transform->SetAbsolutePosition(pos));
			};

		std::function<void(glm::vec3)> p_cam_move_to_func = [this](glm::vec3 pos) {
			mp_editor_camera->GetComponent<TransformComponent>()->SetPosition(pos);
			};

		std::function<void()> p_match_func = [this]() {
			auto* p_cam_transform = mp_editor_camera->GetComponent<TransformComponent>();
			TRANSFORM_LUA_SKELETON(
				p_transform->SetAbsolutePosition(p_cam_transform->GetPosition());
				p_transform->LookAt(p_cam_transform->GetPosition() + p_cam_transform->forward);
			);
			};

		
		std::function<void(glm::vec3, float, glm::vec3)> p_rot_about_point_func = [this](glm::vec3 axis, float angle_degrees, glm::vec3 pivot) {
			TRANSFORM_LUA_SKELETON(
				glm::quat q = glm::angleAxis(glm::radians(angle_degrees), axis);
				glm::vec3 offset_pos = std::get<0>(p_transform->GetAbsoluteTransforms()) - pivot;
				p_transform->SetAbsolutePosition(q * offset_pos + pivot);
				p_transform->SetOrientation(glm::degrees(glm::eulerAngles(glm::angleAxis(glm::radians(angle_degrees), axis) * glm::quat(glm::radians(p_transform->GetOrientation())))))
				);
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

	

	void EditorLayer::RenderPhysxDebug() {
		m_res.p_highlight_shader->ActivateProgram();
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		m_res.p_highlight_shader->SetUniform("u_color", glm::vec4(0.0, 0.75, 0, 0.5));

		auto* p_line_pos_buf = m_res.line_vao.GetBuffer<VertexBufferGL<glm::vec3>>(0);
		p_line_pos_buf->data.clear();

		/*
			Render joints
		*/

		for (auto [entity, joint, transform] : SCENE->m_registry.view<JointComponent, TransformComponent>().each()) {
			for (auto& [p_joint, attachment] : joint.attachments) {
				if (!attachment.p_joint->p_joint || attachment.p_joint->GetA0() != SCENE->GetEntity(entity))
					continue;

				auto pose0 = attachment.p_joint->p_joint->getLocalPose(PxJointActorIndex::eACTOR0);
				auto pose1 = attachment.p_joint->p_joint->getLocalPose(PxJointActorIndex::eACTOR1);

				auto* p_transform0 = attachment.p_joint->GetA0()->GetComponent<TransformComponent>();
				auto* p_transform1 = attachment.p_joint->GetA1()->GetComponent<TransformComponent>();

				auto abs_pos0 = p_transform0->GetAbsPosition();
				auto abs_pos1 = p_transform1->GetAbsPosition();

				glm::vec3 pos0 = abs_pos0 + glm::quat(glm::radians(p_transform0->GetAbsOrientation())) * ConvertVec3<glm::vec3>(pose0.p);
				glm::vec3 pos1 = abs_pos1 + glm::quat(glm::radians(p_transform1->GetAbsOrientation())) * ConvertVec3<glm::vec3>(pose1.p);

				if (m_state.p_selected_joint == p_joint)
					m_res.p_highlight_shader->SetUniform("u_color", glm::vec4(0.1, 0.3, 1.0, 0.75));

				m_res.p_highlight_shader->SetUniform("transform", glm::translate(abs_pos0) * glm::scale(glm::vec3{ 0.1, 0.1, 0.1 }));
				Renderer::DrawSphere();
				m_res.p_highlight_shader->SetUniform("transform", glm::translate(abs_pos1) * glm::scale(glm::vec3{ 0.1, 0.1, 0.1 }));
				Renderer::DrawSphere();
				m_res.p_highlight_shader->SetUniform("transform", glm::translate(pos0) * glm::scale(glm::vec3{ 0.1, 0.1, 0.1 }));
				Renderer::DrawSphere();

				if (m_state.p_selected_joint == p_joint) { // Highlight currently selected joint a different colour
					m_res.p_highlight_shader->SetUniform("u_color", glm::vec4(0, 0.75, 0, 0.5));
					// Draw accumulated lines so far
					m_res.line_vao.FillBuffers();
					glLineWidth(10.f);
					m_res.p_highlight_shader->SetUniform("transform", glm::identity<glm::mat4>());
					Renderer::DrawVAOArrays(m_res.line_vao, p_line_pos_buf->data.size(), GL_LINES);
					p_line_pos_buf->data.clear();

					PushBackMultiple(p_line_pos_buf->data, abs_pos0, pos0, abs_pos1, pos1);

					m_res.p_highlight_shader->SetUniform("u_color", glm::vec4(0.1, 0.3, 1.0, 0.75));
					m_res.line_vao.FillBuffers();
					glLineWidth(10.f);
					m_res.p_highlight_shader->SetUniform("transform", glm::identity<glm::mat4>());
					Renderer::DrawVAOArrays(m_res.line_vao, p_line_pos_buf->data.size(), GL_LINES);
					p_line_pos_buf->data.clear();

					m_res.p_highlight_shader->SetUniform("u_color", glm::vec4(0, 0.75, 0, 0.5));
					continue;
				}

				PushBackMultiple(p_line_pos_buf->data, abs_pos0, pos0, abs_pos1, pos1);
			}
		}

		m_res.line_vao.FillBuffers();
		glLineWidth(10.f);
		m_res.p_highlight_shader->SetUniform("transform", glm::identity<glm::mat4>());
		Renderer::DrawVAOArrays(m_res.line_vao, p_line_pos_buf->data.size(), GL_LINES);
		
		/*
			Render joint makers joint in progress	
		*/

		if (m_state.general_settings.editor_window_settings.display_joint_maker && !m_state.selected_entity_ids.empty()) {
			auto* p_cam = SCENE->GetActiveCamera()->GetEntity();
			auto* p_cam_comp = p_cam->GetComponent<CameraComponent>();
			auto* p_cam_transform = p_cam->GetComponent<TransformComponent>();

			//xtraMath::ScreenCoordsToRayDir(p_cam->GetProjectionMatrix(), min, pos, p_transform->forward, p_transform->up, Window::GetWidth(), Window::GetHeight());
			auto coords = ConvertFullscreenMouseToDisplayMouse(Input::GetMousePos());
			coords.y = Window::GetHeight() - coords.y;

			auto dir = ExtraMath::ScreenCoordsToRayDir(p_cam_comp->GetProjectionMatrix(), coords,
				p_cam_transform->GetAbsPosition(), p_cam_transform->forward, p_cam_transform->up, Window::GetWidth(), Window::GetHeight());

			auto res = SCENE->physics_system.Raycast(p_cam_transform->GetAbsPosition(), dir, p_cam_comp->zFar);

			if (auto* p_ent = res.p_entity) {
				PushBackMultiple(p_line_pos_buf->data, p_ent->GetComponent<TransformComponent>()->GetAbsPosition(),
					SCENE->GetEntity(m_state.selected_entity_ids[m_state.selected_entity_ids.size() - 1])->GetComponent<TransformComponent>()->GetAbsPosition());

				m_res.line_vao.FillBuffers();
				Renderer::DrawVAOArrays(m_res.line_vao, p_line_pos_buf->data.size(), GL_LINES);
				p_line_pos_buf->data.clear();
			}
		}
		glLineWidth(3.f);

		/*
			Render colliders		
		*/

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		// Break into 3 sep. loops to reduce vao changes
		for (auto [entity, phys, transform] : SCENE->m_registry.view<PhysicsComponent, TransformComponent>().each()) {
			if (phys.m_geometry_type == PhysicsComponent::BOX) {
				if (auto* p_mesh = phys.GetEntity()->GetComponent<MeshComponent>())
					m_res.p_highlight_shader->SetUniform("transform", transform.GetMatrix() * glm::scale(p_mesh->GetMeshData()->GetAABB().max * 2.f));
				else
					m_res.p_highlight_shader->SetUniform("transform", transform.GetMatrix());

				Renderer::DrawCube();
			}
		}

		for (auto [entity, phys, transform] : SCENE->m_registry.view<PhysicsComponent, TransformComponent>().each()) {
			if (phys.m_geometry_type == PhysicsComponent::SPHERE) {
				auto [t, s, r] = transform.GetAbsoluteTransforms();
				auto sf = ((PxSphereGeometry*)&phys.p_shape->getGeometry())->radius;
				glm::mat4 m;
				glm::mat4x4 rot_mat = ExtraMath::Init3DRotateTransform(transform.m_orientation.x, transform.m_orientation.y, transform.m_orientation.z);

				if (transform.m_is_absolute || !transform.GetParent()) {
					glm::mat4x4 scale_mat = ExtraMath::Init3DScaleTransform(sf, sf, sf);
					glm::mat4x4 trans_mat = ExtraMath::Init3DTranslationTransform(transform.m_pos.x, transform.m_pos.y, transform.m_pos.z);
					m = trans_mat * rot_mat * scale_mat;
				}
				else {
					s /= transform.m_scale;
					// Apply parent scaling to the position and scale to make up for not using the scale transform below
					glm::mat4x4 scale_mat = ExtraMath::Init3DScaleTransform(sf, sf, sf);
					glm::mat4x4 trans_mat = ExtraMath::Init3DTranslationTransform(transform.m_pos.x * s.x, transform.m_pos.y * s.y, transform.m_pos.z * s.z);
					//m = transform.GetParent()->GetMatrix() * glm::inverse(ExtraMath::Init3DScaleTransform(s.x, s.y, s.z)) * (trans_mat * rot_mat * scale_mat);
				}

				// Undo scaling to prevent shearing
				m_res.p_highlight_shader->SetUniform("transform", m);
				Renderer::DrawSphere();
			}
		}

		for (auto [entity, mesh, phys, transform] : SCENE->m_registry.view<MeshComponent, PhysicsComponent, TransformComponent>().each()) {
			if (phys.m_geometry_type == PhysicsComponent::TRIANGLE_MESH) {
				m_res.p_highlight_shader->SetUniform("transform", transform.GetMatrix());
				for (int i = 0; i < mesh.mp_mesh_asset->m_submeshes.size(); i++) {
					Renderer::DrawSubMesh(mesh.mp_mesh_asset, i);
				}
			}
		}

		m_res.p_raymarch_shader->Activate((unsigned)EditorResources::RaymarchSV::CAPSULE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		for (auto [entity, controller, transform] : SCENE->m_registry.view<CharacterControllerComponent, TransformComponent>().each()) {
			PxCapsuleController* p_controller = static_cast<PxCapsuleController*>(controller.p_controller);
			m_res.p_raymarch_shader->SetUniform("u_capsule_pos", transform.GetAbsPosition());
			m_res.p_raymarch_shader->SetUniform<float>("u_capsule_height", p_controller->getHeight());
			m_res.p_raymarch_shader->SetUniform<float>("u_capsule_radius", p_controller->getRadius());
			Renderer::DrawQuad();
		}

		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
	}




	void EditorLayer::RenderSkyboxEditor() {
		if (ExtraUI::H1TreeNode("Skybox")) {
			wchar_t valid_extensions[MAX_PATH] = L"Texture Files: *.png;*.jpg;*.jpeg;*.hdr\0*.png;*.jpg;*.jpeg;*.hdr\0";

			static bool using_env_maps = true;
			static unsigned resolution = 4096;

			using_env_maps = SCENE->skybox.using_env_map;
			resolution = SCENE->skybox.m_resolution;

			std::function<void(std::string)> file_explorer_callback = [this](std::string filepath) {
				// Check if texture is an asset or not, if not, add it
				std::string new_filepath = "./res/textures/" + filepath.substr(filepath.find_last_of("\\") + 1);
				if (!std::filesystem::exists(new_filepath)) {
					FileCopy(filepath, new_filepath);
				}
				SCENE->skybox.Load(new_filepath, resolution, using_env_maps);
				};

			if (ImGui::Button("Load skybox texture")) {
				ExtraUI::ShowFileExplorer("", valid_extensions, file_explorer_callback);
			}
			ImGui::SameLine();
			ImGui::SmallButton("?");
			if (ImGui::BeginItemTooltip()) {
				ImGui::Text("Converts an equirectangular image into a cubemap for use in a skybox. For best results, use HDRI's!");
				ImGui::EndTooltip();
			}

			ExtraUI::InputUint("Resolution", SCENE->skybox.m_resolution);

			if (ImGui::Checkbox("Gen IBL textures", &using_env_maps) || ImGui::Button("Reload")) {
				SCENE->skybox.Load(SCENE->skybox.GetSrcFilepath(), resolution, using_env_maps);
			}
		}
	}




	EntityNodeData EditorLayer::RenderEntityNode(SceneEntity* p_entity, unsigned int layer, bool node_selection_active, const Box2D& selection_box) {
		EntityNodeData ret{ EntityNodeEvent::E_NONE, {0, 0}, {0, 0} };

		// Tree nodes that are open are stored here so their children are rendered with the logic below, independent of if the parent tree node is visible or not.

		static std::string padding_str;
		for (int i = 0; i < layer; i++) {
			padding_str += " |--|";
		}
		ImGui::Text(padding_str.c_str());
		padding_str.clear();
		ImGui::SameLine();
		// Setup display name with icons
		static std::string formatted_name; // Static to stop a new string being made every single call, only needed for c_str anyway
		formatted_name.clear();

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
			formatted_name += " ";
			formatted_name += p_entity->name;
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
				SelectEntity(p_entity->uuid());
			else if (!Input::IsKeyDown(Key::LeftControl))
				DeselectEntity(p_entity->uuid());
		}

		if (ImGui::IsItemToggledOpen()) {
			auto it = std::ranges::find(m_state.open_tree_nodes_entities, p_entity->GetUUID());
			if (it == m_state.open_tree_nodes_entities.end())
				m_state.open_tree_nodes_entities.push_back(p_entity->GetUUID());
			else
				m_state.open_tree_nodes_entities.erase(it);
		}

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("ENTITY")) {
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
				if (!Input::IsKeyDown(GLFW_KEY_LEFT_CONTROL)) // Only selecting one entity at a time
					m_state.selected_entity_ids.clear();

				if (Input::IsKeyDown(GLFW_KEY_LEFT_CONTROL) && VectorContains(m_state.selected_entity_ids, p_entity->GetUUID()) && ImGui::IsMouseClicked(0)) // Deselect entity from group of entities currently selected
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
		// Render entity nodes for all the children of this entity
		if (p_entity && VectorContains(m_state.open_tree_nodes_entities, p_entity->GetUUID())) {
			entt::entity current_child_entity = p_entity_relationship_comp->first;
			while (current_child_entity != entt::null) {
				auto& child_rel_comp = SCENE->m_registry.get<RelationshipComponent>(current_child_entity);
				// Render child entity node, propagate event up
				ret.e_event = static_cast<EntityNodeEvent>((ret.e_event | RenderEntityNode(child_rel_comp.GetEntity(), layer + 1, node_selection_active, selection_box).e_event));
				current_child_entity = child_rel_comp.next;
			}
		}

		ImGui::PopID();
		return ret;
	}




	void EditorLayer::RenderSceneGraph() {
		static bool mouse_over_title = false;

		if (ImGui::Begin("Scene graph", (bool*)0, ImGuiWindowFlags_NoNavInputs | (mouse_over_title ? 0 : ImGuiWindowFlags_NoMove))) {
			if (!Input::IsMouseDown(0))
				mouse_over_title = ImGui::IsItemHovered();
			// Click anywhere on window to deselect entity nodes
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !Input::IsKeyDown(GLFW_KEY_LEFT_CONTROL))
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
				if (ImGui::Begin("##sel-box", (bool*)0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs)) {
					ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0, 1, 0.1 });
					ImGui::Button("##blue", { node_selection_box.max.x - node_selection_box.min.x,  node_selection_box.max.y - node_selection_box.min.y });
					ImGui::PopStyleColor();
				}
				ImGui::End();
			}

			ImGui::Text("Editor cam exposure");
			ImGui::SliderFloat("##exposure", &mp_editor_camera->GetComponent<CameraComponent>()->exposure, 0.f, 10.f);

			EntityNodeEvent active_event = EntityNodeEvent::E_NONE;

			if (ExtraUI::H2TreeNode("Scene settings")) {
				m_state.general_settings.editor_window_settings.display_skybox_editor = ExtraUI::EmptyTreeNode("Skybox");
				m_state.general_settings.editor_window_settings.display_directional_light_editor = ExtraUI::EmptyTreeNode("Directional Light");
				m_state.general_settings.editor_window_settings.display_global_fog_editor = ExtraUI::EmptyTreeNode("Global fog");
				m_state.general_settings.editor_window_settings.display_terrain_editor = ExtraUI::EmptyTreeNode("Terrain");
				m_state.general_settings.editor_window_settings.display_bloom_editor = ExtraUI::EmptyTreeNode("Bloom");
			}

			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2);
			ImGui::PushStyleColor(ImGuiCol_Header, m_res.lightest_grey_color);
			if (ExtraUI::H2TreeNode("Entities")) {
				for (auto* p_entity : SCENE->m_entities) {
					if (p_entity->GetComponent<RelationshipComponent>()->parent != entt::null)
						continue;

					auto node_data = RenderEntityNode(p_entity, 0, node_selection_active, node_selection_box);

					active_event = (EntityNodeEvent)(node_data.e_event | active_event);
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
					if (auto* p_entity = SCENE->GetEntity(id))
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
		} // begin "scene graph"

		ImGui::End();
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
		static bool mouse_over_title = false;

		if (ImGui::Begin("Properties", (bool*)0, ImGuiWindowFlags_NoNavInputs| ImGuiWindowFlags_AlwaysAutoResize | (mouse_over_title ? 0 : ImGuiWindowFlags_NoMove))) {
			if (!Input::IsMouseDown(0))
				mouse_over_title = ImGui::IsItemHovered();

			if (m_state.general_settings.editor_window_settings.display_directional_light_editor)
				RenderDirectionalLightEditor();
			if (m_state.general_settings.editor_window_settings.display_global_fog_editor)
				RenderGlobalFogEditor();
			if (m_state.general_settings.editor_window_settings.display_skybox_editor)
				RenderSkyboxEditor();
			if (m_state.general_settings.editor_window_settings.display_terrain_editor)
				RenderTerrainEditor();
			if (m_state.general_settings.editor_window_settings.display_bloom_editor)
				RenderBloomEditor();

			auto entity = SCENE->GetEntity(m_state.selected_entity_ids.empty() ? 0 : m_state.selected_entity_ids[0]);
			if (!entity) {
				ImGui::End();
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

			RenderCompEditor<TransformComponent>(entity, "Transform component", [this](TransformComponent* p_comp) { RenderTransformComponentEditor(transforms); });
			RenderCompEditor<MeshComponent>(entity, "Mesh component", [this](MeshComponent* p_comp) { RenderMeshComponentEditor(p_comp); });
			RenderCompEditor<PointLightComponent>(entity, "Pointlight component", [this](PointLightComponent* p_comp) { RenderPointlightEditor(p_comp); });
			RenderCompEditor<SpotLightComponent>(entity, "Spotlight component", [this](SpotLightComponent* p_comp) { RenderSpotlightEditor(p_comp); });
			RenderCompEditor<CameraComponent>(entity, "Camera component", [this](CameraComponent* p_comp) { RenderCameraEditor(p_comp); });
			RenderCompEditor<PhysicsComponent>(entity, "Physics component", [this](PhysicsComponent* p_comp) { RenderPhysicsComponentEditor(p_comp); });
			RenderCompEditor<ScriptComponent>(entity, "Script component", [this](ScriptComponent* p_comp) { RenderScriptComponentEditor(p_comp); });
			RenderCompEditor<AudioComponent>(entity, "Audio component", [this](AudioComponent* p_comp) { RenderAudioComponentEditor(p_comp); });
			RenderCompEditor<VehicleComponent>(entity, "Vehicle component", [this](VehicleComponent* p_comp) { RenderVehicleComponentEditor(p_comp); });
			RenderCompEditor<ParticleEmitterComponent>(entity, "Particle emitter component", [this](ParticleEmitterComponent* p_comp) { RenderParticleEmitterComponentEditor(p_comp); });
			RenderCompEditor<ParticleBufferComponent>(entity, "Particle buffer component", [this](ParticleBufferComponent* p_comp) { RenderParticleBufferComponentEditor(p_comp); });
			RenderCompEditor<CharacterControllerComponent>(entity, "Character controller component", [this](CharacterControllerComponent* p_comp) { RenderCharacterControllerComponentEditor(p_comp); });
			RenderCompEditor<JointComponent>(entity, "Joint component", [this](JointComponent* p_comp) { RenderJointComponentEditor(p_comp); });


			glm::vec2 window_size = { ImGui::GetWindowSize().x, ImGui::GetWindowSize().y };
			glm::vec2 button_size = { 200, 50 };
			glm::vec2 padding_size = { (window_size.x / 2.f) - button_size.x / 2.f, 50.f };
			ImGui::Dummy(ImVec2(padding_size.x, padding_size.y));

		}

		ImGui::Dummy({ 0, 50 });
		ImGui::End(); // end entity editor window
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
		if (p_script->p_instance) {
			ImGui::PushItemWidth(275.f);

			for (auto& [name, mem] : p_script->p_instance->m_member_addresses) {
				ImGui::PushID(&mem);

				if (auto* p_val = AnyCast<unsigned>(mem)) {
					static int i = *p_val;
					i = *p_val;
					ImGui::Text(name.c_str()); ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 285.f);
					if (ImGui::InputInt("##sv", &i))
						*p_val = i;
				}
				else if (auto* p_val = AnyCast<int>(mem)) {
					ImGui::Text(name.c_str()); ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 285.f);
					ImGui::InputInt("##sv", p_val);
				}
				else if (auto* p_val = AnyCast<float>(mem)) {
					ImGui::Text(name.c_str()); ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 285.f);
					ImGui::InputFloat("##sv", p_val);
				}
				else if (auto* p_val = AnyCast<bool>(mem)) {
					ImGui::Text(name.c_str()); ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 285.f);
					ImGui::Checkbox("##sv", p_val);
				}
				else if (auto* p_val = AnyCast<glm::vec2>(mem)) {
					ImGui::Text(name.c_str()); ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 285.f);
					ImGui::InputFloat2("##sv", &(*p_val)[0]);
				}
				else if (auto* p_val = AnyCast<glm::vec3>(mem)) {
					ImGui::Text(name.c_str()); ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 285.f);
					ImGui::InputFloat3("##sv", &(*p_val)[0]);
				}
				else if (auto* p_val = AnyCast<glm::vec4>(mem)) {
					ImGui::Text(name.c_str()); ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 285.f);
					ImGui::InputFloat4("##sv", &(*p_val)[0]);
				}
				ImGui::PopID();
			}
			ImGui::PopItemWidth();
		}



		ExtraUI::NameWithTooltip(p_script->script_filepath.substr(p_script->script_filepath.find_last_of("\\") + 1).c_str());
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

	void EditorLayer::RenderVehicleComponentEditor(VehicleComponent* p_comp) {
		ImGui::SeparatorText("Body mesh");
		ImGui::ImageButton(ImTextureID(m_asset_manager_window.GetMeshPreviewTex(p_comp->p_body_mesh)), ImVec2(100, 100), ImVec2(0, 1), ImVec2(1, 0));
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("MESH")) {
				if (p_payload->DataSize == sizeof(MeshAsset*)) {
					p_comp->p_body_mesh = *static_cast<MeshAsset**>(p_payload->Data);
					p_comp->m_body_materials = { p_comp->p_body_mesh->num_materials, AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID) };
				}
			}
			ImGui::EndDragDropTarget();
		}
		ImGui::Text("Scale"); ImGui::SameLine();
		ImGui::DragFloat3("##s1", &p_comp->body_scale[0]);

		ImGui::SeparatorText("Wheel mesh");
		ImGui::ImageButton(ImTextureID(m_asset_manager_window.GetMeshPreviewTex(p_comp->p_wheel_mesh)), ImVec2(100, 100), ImVec2(0, 1), ImVec2(1, 0));
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("MESH")) {
				if (p_payload->DataSize == sizeof(MeshAsset*)) {
					p_comp->p_wheel_mesh = *static_cast<MeshAsset**>(p_payload->Data);
					p_comp->m_wheel_materials = { p_comp->p_wheel_mesh->num_materials, AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID) };
				}
			}
			ImGui::EndDragDropTarget();
		}
		ImGui::Text("Scale"); ImGui::SameLine();
		ImGui::DragFloat3("##s2", &p_comp->wheel_scale[0]);

		ImGui::SeparatorText("Body materials");
		for (int i = 0; i < p_comp->m_body_materials.size(); i++) {
			auto p_material = p_comp->m_body_materials[i];
			ImGui::PushID(&i);

			if (auto* p_new_material = RenderMaterialComponent(p_material)) {
				p_comp->m_body_materials[i] = p_new_material;
			}

			ImGui::PopID();
		}


		ImGui::SeparatorText("Wheel materials");
		for (int i = 0; i < p_comp->m_wheel_materials.size(); i++) {
			auto p_material = p_comp->m_wheel_materials[i];
			ImGui::PushID(&i);

			if (auto* p_new_material = RenderMaterialComponent(p_material)) {
				p_comp->m_wheel_materials[i] = p_new_material;
			}

			ImGui::PopID();
		}

		for (int i = 0; i < 4; i++) {
			ImGui::PushID(i);
			if (ImGui::DragFloat3("##pos", &p_comp->m_vehicle.mBaseParams.suspensionParams[i].suspensionAttachment.p[0])) {
				SCENE->physics_system.InitVehicle(p_comp);
			}
			ImGui::PopID();
		}
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

	void EditorLayer::RenderCharacterControllerComponentEditor(CharacterControllerComponent* p_comp) {
		ImGui::PushID(p_comp);
		PxCapsuleController* p_capsule = static_cast<PxCapsuleController*>(p_comp->p_controller);
		static float radius = 0.f;
		radius = p_capsule->getRadius();
		static float height = 0.f;
		height = p_capsule->getHeight();
		
		ImGui::Text("Radius"); ImGui::SameLine();
		ImGui::InputFloat("##radius", &radius);

		ImGui::Text("Height"); ImGui::SameLine();
		ImGui::InputFloat("##height", &height);

		p_capsule->setHeight(height);
		p_capsule->setRadius(radius);

		ImGui::PopID();
	}

	void EditorLayer::RenderEntityNodeRef(EntityNodeRef& ref) {
		ImGui::PushID(&ref);

		auto* p_ent = SCENE->TryFindEntity(ref);

		ImGui::PopID();
	}

	void EditorLayer::RenderJointMaker() {
		static JointComponent::Joint joint{ nullptr };
		if (ImGui::Begin("Joint maker")) {
			RenderJointEditor(&joint);

			if (m_state.item_selected_this_frame && m_state.selected_entity_ids.size() >= 2) {
				auto* p_ent0 = SCENE->GetEntity(m_state.selected_entity_ids[m_state.selected_entity_ids.size() - 2]);
				auto* p_ent1 = SCENE->GetEntity(m_state.selected_entity_ids[m_state.selected_entity_ids.size() - 1]);
				auto* p_comp0 = p_ent0->AddComponent<JointComponent>();
				auto* p_comp1 = p_ent1->AddComponent<JointComponent>();

				if (!(p_comp0 && p_comp1 && p_ent0->HasComponent<PhysicsComponent>() && p_ent1->HasComponent<PhysicsComponent>())) {
					ImGui::End();
					return;
				}
					
				auto& attachment = p_comp0->CreateJoint();
				*attachment.p_joint = joint;
				attachment.p_joint->p_a0 = p_ent0;

				attachment.p_joint->Connect(p_comp1);
			}
		}

		ImGui::End();
	}

	void EditorLayer::RenderJointEditor(JointComponent::Joint* p_joint) {
		// Key: PxD6Axis, Val: is_axis_free
		static std::unordered_map<PxD6Axis::Enum, bool> motion;
		std::array<const char*, 6> motion_strings = { "X", "Y", "Z", "TWIST", "SWING1", "SWING2" };
		static std::array<float, 2> break_force;

		break_force[0] = p_joint->force_threshold;
		break_force[1] = p_joint->torque_threshold;
		for (int i = 0; i < 6; i++) {
			auto axis = (PxD6Axis::Enum)i;
			motion[axis] = (p_joint->motion[axis] == PxD6Motion::eFREE);
		}

		ImGui::SeparatorText("Motion freedom");
		for (int i = 0; i < 6; i++) {
			auto axis = (PxD6Axis::Enum)i;
			if (ImGui::Checkbox(motion_strings[i], &motion[axis])) {
				p_joint->SetMotion(axis, motion[axis] ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
			}

			if (i != 2 && i != 5)
				ImGui::SameLine();
		}

		ImGui::SeparatorText("Break force");
		bool break_force_changed = false;
		ImGui::Text("Force"); ImGui::SameLine();
		break_force_changed |= ImGui::InputFloat("##f", &break_force[0]);
		ImGui::Text("Torque"); ImGui::SameLine();
		break_force_changed |= ImGui::InputFloat("##t", &break_force[1]);

		if (break_force_changed) {
			p_joint->SetBreakForce(glm::max(break_force[0], 0.f), glm::max(break_force[1], 0.f));
		}

		static glm::vec3 vec0;
		vec0 = p_joint->poses[0];
		static glm::vec3 vec1;
		vec1 = p_joint->poses[1];

		if (ExtraUI::ShowVec3Editor("Local pose 0", vec0)) {
			p_joint->SetLocalPose(0, vec0);
		}
		if (ExtraUI::ShowVec3Editor("Local pose 1", vec1)) {
			p_joint->SetLocalPose(1, vec1);
		}
	}


	void EditorLayer::RenderJointComponentEditor(JointComponent* p_comp) {
		ImGui::PushID(p_comp);

		// Key: PxD6Axis, Val: is_axis_free
		static std::unordered_map<PxD6Axis::Enum, bool> motion;
		std::array<const char*, 6> motion_strings = {"X", "Y", "Z", "TWIST", "SWING1", "SWING2"};

		static std::array<float, 2> break_force;

		for (auto& [p_joint, attachment] : p_comp->attachments) {
			if (!p_joint->GetA1())
				continue;

			ImGui::PushID(p_joint);

			bool owns_joint = p_joint->GetA0() == p_comp->GetEntity();
			bool joint_tree_node_open = ImGui::TreeNode(std::string((owns_joint ? "[A0] " + p_joint->GetA1()->name : "[A1] " + p_joint->GetA0()->name)).c_str());

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(1)) {
				p_joint->Break();
				ImGui::TreePop();
				ImGui::PopID();

				if (joint_tree_node_open)
					ImGui::PopID();

				// Break here as the loop is now invalid, causes a 1-frame flicker
				return;
			}

			if (!joint_tree_node_open) {
				ImGui::PopID();
				continue;
			}

			RenderJointEditor(p_joint);

			ImGui::TreePop();
			ImGui::PopID();
		}

		if (!m_state.p_selected_joint || m_state.selection_mode != SelectionMode::JOINT || !m_state.p_selected_joint->GetA1()) {
			ImGui::PopID();
			return;
		}

		ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
		auto* p_cam_transform = mp_editor_camera->GetComponent<TransformComponent>();
		glm::mat4 view_mat = glm::lookAt(p_cam_transform->GetAbsPosition(), p_cam_transform->GetAbsPosition() + p_cam_transform->forward, p_cam_transform->up);

		auto* p_transform0 = m_state.p_selected_joint->GetA0()->GetComponent<TransformComponent>();
		auto* p_transform1 = m_state.p_selected_joint->GetA1()->GetComponent<TransformComponent>();

		glm::quat orientation_q0 = glm::quat(radians(p_transform0->GetAbsOrientation()));
		glm::vec3 matrix_translation = p_transform0->GetAbsPosition() + glm::quat(radians(p_transform0->GetAbsOrientation())) * m_state.p_selected_joint->poses[0];
		glm::mat4 current_operation_matrix = glm::translate(matrix_translation);
		
		if (ImGuizmo::Manipulate(&view_mat[0][0], &mp_editor_camera->GetComponent<CameraComponent>()->GetProjectionMatrix()[0][0], ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::MODE::WORLD, &current_operation_matrix[0][0], nullptr, nullptr) && ImGuizmo::IsUsing()) {
			glm::vec3 manip_translation;
			glm::vec3 manip_scale;
			glm::vec3 manip_orientation;
			ImGuizmo::DecomposeMatrixToComponents(&current_operation_matrix[0][0], &manip_translation[0], &manip_scale[0], &manip_orientation[0]);
			glm::vec3 new_center = manip_translation;

			m_state.p_selected_joint->SetLocalPose(0, inverse(orientation_q0) * (new_center - p_transform0->GetAbsPosition()));
			m_state.p_selected_joint->SetLocalPose(1, glm::inverse(glm::quat(radians(p_transform1->GetAbsOrientation()))) * (new_center - p_transform1->GetAbsPosition()));
		}

		ImGui::PopID();
	}



	void EditorLayer::RenderParticleEmitterComponentEditor(ParticleEmitterComponent* p_comp) {
		ImGui::PushID(p_comp);

		const char* types[2] = { "BILLBOARD", "MESH" };
		ParticleEmitterComponent::EmitterType emitter_types[2] = { ParticleEmitterComponent::EmitterType::BILLBOARD, ParticleEmitterComponent::EmitterType::MESH };
		static int current_item = 0;

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
				p_res->materials = { p_new->GetNbMaterials(), AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID) };
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
			if (ExtraUI::InterpolatorV3Graph("Colour over life", &p_comp->m_life_colour_interpolator))
				p_comp->DispatchUpdateEvent(ParticleEmitterComponent::MODIFIERS_CHANGED);

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Alpha over life")) {
			if (ExtraUI::InterpolatorV1Graph("Alpha over life", &p_comp->m_life_alpha_interpolator))
				p_comp->DispatchUpdateEvent(ParticleEmitterComponent::MODIFIERS_CHANGED);

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Scale over life")) {
			if (ExtraUI::InterpolatorV3Graph("Scale over life", &p_comp->m_life_scale_interpolator))
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
		n = p_comp->m_num_particles;
		if (ImGui::DragInt("##np", &n) && n >= 0 && n <= 100'000) {
			p_comp->SetNbParticles(n);
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
		total_length /= 1000.0;
		position /= 1000.0;

		ImGui::SetCursorPos(prev_curs_pos);
		ExtraUI::ColoredButton("##playback position", m_res.orange_color_dark, ImVec2(playback_widget_width * ((float)position / (float)total_length), playback_widget_height));
		ImGui::PopStyleVar();
		ImGui::SetCursorPos(prev_curs_pos);
		ImGui::Text(std::format("{}:{}", position, total_length).c_str());

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

				p_audio->mp_channel->setPosition((unsigned)((local_mouse.x / playback_widget_width) * (float)total_length) * 1000.0, FMOD_TIMEUNIT_MS);
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

		ImGui::Text(p_sound->source_filepath.substr(p_sound->source_filepath.rfind("\\") + 1).c_str());

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
			ImGui::Text(p_comp->p_material->name.c_str());
			ImGui::Button(ICON_FA_FILE, { 125, 125 });

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("PHYSX-MATERIAL"); p_payload && p_payload->DataSize == sizeof(PhysXMaterialAsset*)) {
					p_comp->SetMaterial(**static_cast<PhysXMaterialAsset**>(p_payload->Data));
				}
			}


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
		glm::vec3 matrix_rotation = transforms[0]->m_orientation;
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
		ImGuiIO& io = ImGui::GetIO();
		ImGuizmo::BeginFrame();
		if (m_state.fullscreen_scene_display)
			ImGuizmo::SetRect(ImGui::GetMainViewport()->Pos.x, ImGui::GetMainViewport()->Pos.y + m_res.toolbar_height, m_state.scene_display_rect.x, m_state.scene_display_rect.y);
		else
			ImGuizmo::SetRect(ImGui::GetMainViewport()->Pos.x + LEFT_WINDOW_WIDTH, ImGui::GetMainViewport()->Pos.y + m_res.toolbar_height, m_state.scene_display_rect.x, m_state.scene_display_rect.y);

		if (m_state.selection_mode != SelectionMode::ENTITY)
			return;

		glm::mat4 current_operation_matrix = transforms[0]->GetMatrix();

		CameraComponent* p_cam = SCENE->m_camera_system.GetActiveCamera();
		auto* p_cam_transform = p_cam->GetEntity()->GetComponent<TransformComponent>();
		glm::vec3 cam_pos = p_cam_transform->GetAbsPosition();
		glm::mat4 view_mat = glm::lookAt(cam_pos, cam_pos + p_cam_transform->forward, p_cam_transform->up);

		glm::mat4 delta_matrix;
		ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
		static glm::vec3 snap = glm::vec3(0.01f);

		static bool is_using = false;
		static bool mouse_down = false;

		if (ImGuizmo::Manipulate(&view_mat[0][0], &p_cam->GetProjectionMatrix()[0][0], m_state.current_gizmo_operation, m_state.current_gizmo_mode, &current_operation_matrix[0][0], &delta_matrix[0][0], &snap[0]) && ImGuizmo::IsUsing()) {
			if (!is_using && !mouse_down && !m_state.simulate_mode_active) {
				EditorEntityEvent e{ TRANSFORM_UPDATE, m_state.selected_entity_ids };
				for (auto id : m_state.selected_entity_ids) {
					e.serialized_entities_before.push_back(SceneSerializer::SerializeEntityIntoString(*SCENE->GetEntity(id)));
				}
				m_event_stack.PushEvent(e);
			}

			ImGuizmo::DecomposeMatrixToComponents(&delta_matrix[0][0], &matrix_translation[0], &matrix_rotation[0], &matrix_scale[0]);

			// The origin of the transform (rotate about this point)
			auto [base_abs_translation, base_abs_scale, base_abs_orientation] = transforms[0]->GetAbsoluteTransforms();

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
				{
					p_transform->SetScale(p_transform->m_scale * delta_scale);
					break;
				}
				case ImGuizmo::ROTATE: // This will rotate multiple objects as one, using entity transform at m_state.selected_entity_ids[0] as origin
					if (auto* p_parent_transform = p_transform->GetParent()) {
						glm::vec3 s = p_parent_transform->GetAbsScale();
						glm::mat4 rot = glm::mat4(glm::inverse(ExtraMath::Init3DScaleTransform(s.x, s.y, s.z)));
						rot[3][3] = 1.0;

						glm::mat3 to_parent_space = p_parent_transform->GetMatrix() * rot;
						glm::vec3 local_rot = glm::inverse(to_parent_space) * glm::vec4(delta_rotation, 0.0);
						glm::vec3 total = glm::eulerAngles(glm::quat(glm::radians(local_rot)) * glm::quat(glm::radians(p_transform->m_orientation)));
						p_transform->SetOrientation(glm::degrees(total));
					}
					else {
						auto orientation = glm::degrees(glm::eulerAngles(glm::quat(glm::radians(delta_rotation)) * glm::quat(glm::radians(p_transform->m_orientation))));
						p_transform->SetOrientation(orientation.x, orientation.y, orientation.z);
					}

					glm::vec3 abs_translation = p_transform->GetAbsPosition();
					glm::vec3 transformed_pos = abs_translation - base_abs_translation;
					glm::vec3 rotation_offset = glm::mat3(ExtraMath::Init3DRotateTransform(delta_rotation.x, delta_rotation.y, delta_rotation.z)) * transformed_pos; // rotate around transformed origin
					p_transform->SetAbsolutePosition(base_abs_translation + rotation_offset);
					break;
				}
			}

			is_using = true;
			mouse_down = Input::IsMouseDown(0);
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

		if (ImGui::ImageButton(ImTextureID(m_asset_manager_window.GetMaterialPreviewTex(p_material)), ImVec2(100, 100), ImVec2(0, 1), ImVec2(1, 0))) {
			m_asset_manager_window.SelectMaterial(AssetManager::GetAsset<Material>(p_material->uuid()));
		};

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("MATERIAL")) {
				if (p_payload->DataSize == sizeof(Material*))
					ret = *static_cast<Material**>(p_payload->Data);
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::Text(p_material->name.c_str());
		return ret;
	}




	void EditorLayer::RenderMeshWithMaterials(const MeshAsset* p_asset, std::vector<const Material*>& materials, std::function<void(MeshAsset* p_new)> OnMeshDrop, std::function<void(unsigned index, Material* p_new)> OnMaterialDrop) {
		ImGui::PushID(p_asset);

		ImGui::SeparatorText("Mesh");
		ImGui::ImageButton(ImTextureID(m_asset_manager_window.GetMeshPreviewTex(p_asset)), ImVec2(100, 100), ImVec2(0, 1), ImVec2(1, 0));
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("MESH")) {
				if (p_payload->DataSize == sizeof(MeshAsset*)) {
					OnMeshDrop(*static_cast<MeshAsset**>(p_payload->Data));
				}
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::SeparatorText("Materials");
		for (int i = 0; i < materials.size(); i++) {
			auto p_material = materials[i];
			ImGui::PushID(i);

			if (auto* p_new_material = RenderMaterialComponent(p_material)) {
				OnMaterialDrop(i, p_new_material);
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

		ExtraUI::ShowColorVec3Editor("Color", light->color);

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
			ExtraUI::ShowColorVec3Editor("Color", SCENE->post_processing.global_fog.color);
		}
	}




	void EditorLayer::RenderBloomEditor() {
		if (ExtraUI::H2TreeNode("Bloom")) {
			ImGui::SliderFloat("Intensity", &SCENE->post_processing.bloom.intensity, 0.f, 10.f);
			ImGui::SliderFloat("Threshold", &SCENE->post_processing.bloom.threshold, 0.0f, 50.0f);
			ImGui::SliderFloat("Knee", &SCENE->post_processing.bloom.knee, 0.f, 1.f);
		}
	}




	void EditorLayer::RenderTerrainEditor() {
		if (ExtraUI::H2TreeNode("Terrain")) {
			ImGui::InputFloat("Height factor", &SCENE->terrain.m_height_scale);
			static int terrain_width = 1000;

			if (ImGui::InputInt("Size", &terrain_width) && terrain_width > 500)
				SCENE->terrain.m_width = terrain_width;
			else
				terrain_width = SCENE->terrain.m_width;

			static int terrain_seed = 123;
			ImGui::InputInt("Seed", &terrain_seed);
			SCENE->terrain.m_seed = terrain_seed;

			if (auto* p_new_material = RenderMaterialComponent(SCENE->terrain.mp_material))
				SCENE->terrain.mp_material = p_new_material;

			if (ImGui::Button("Reload"))
				SCENE->terrain.ResetTerrainQuadtree();
		}
	}




	void EditorLayer::RenderDirectionalLightEditor() {
		if (ExtraUI::H1TreeNode("Directional light")) {
			ImGui::Text("DIR LIGHT CONTROLS");

			static glm::vec3 light_dir = SCENE->directional_light.m_light_direction;
			static glm::vec3 light_color = SCENE->directional_light.color;

			ImGui::SliderFloat("X", &light_dir.x, -1.f, 1.f);
			ImGui::SliderFloat("Y", &light_dir.y, -1.f, 1.f);
			ImGui::SliderFloat("Z", &light_dir.z, -1.f, 1.f);
			ExtraUI::ShowColorVec3Editor("Color", light_color);

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

			SCENE->directional_light.color = glm::vec3(light_color.x, light_color.y, light_color.z);
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
		ExtraUI::ShowColorVec3Editor("Color", light->color);
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

			e.affected_entities.push_back(p_dup_ent->uuid());

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
				e.affected_entities.push_back(p_ent->uuid());

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
			if (!VectorContains(m_state.open_tree_nodes_entities, p_current_parent->uuid()))
				m_state.open_tree_nodes_entities.push_back(p_current_parent->uuid());

			p_current_parent = SCENE->GetEntity(p_current_parent->GetParent());
		}

		m_state.item_selected_this_frame = true;
	}

	void EditorLayer::DeselectEntity(uint64_t id) {
		auto it = std::ranges::find(m_state.selected_entity_ids, id);
		if (it != m_state.selected_entity_ids.end())
			m_state.selected_entity_ids.erase(it);
	}

	void EditorLayer::SetScene(std::unique_ptr<Scene>* p_scene) {
		mp_scene_context = p_scene;
		m_asset_manager_window.SetScene(p_scene);
	}
}