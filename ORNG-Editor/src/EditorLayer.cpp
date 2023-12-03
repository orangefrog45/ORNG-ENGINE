#include "pch/pch.h"

#include <glfw/glfw3.h>
#include <fmod.hpp>

#include "EditorLayer.h"
#include "../extern/Icons.h"
#include "../extern/imgui/backends/imgui_impl_glfw.h"
#include "../extern/imgui/backends/imgui_impl_opengl3.h"
#include "scene/SceneSerializer.h"
#include "util/Timers.h"
#include "../extern/imguizmo/ImGuizmo.h"
#include "core/CodedAssets.h"
#include "assets/AssetManager.h"
#include "yaml-cpp/yaml.h"
#include "scripting/ScriptingEngine.h"
#include "core/Input.h"
#include "cudamanager/PxCudaContext.h"
#include "util/ExtraUI.h"


constexpr unsigned LEFT_WINDOW_WIDTH = 75;
constexpr unsigned RIGHT_WINDOW_WIDTH = 450;


namespace ORNG {
	void EditorLayer::Init() {
		char buffer[ORNG_MAX_FILEPATH_SIZE];
		GetModuleFileName(nullptr, buffer, ORNG_MAX_FILEPATH_SIZE);
		m_executable_directory = buffer;
		m_executable_directory = m_executable_directory.substr(0, m_executable_directory.find_last_of('\\'));

		InitImGui();
		m_active_scene = std::make_unique<Scene>();
		m_asset_manager_window.Init();

		m_grid_mesh = std::make_unique<GridMesh>();
		m_grid_mesh->Init();
		mp_grid_shader = &Renderer::GetShaderLibrary().CreateShader("grid");
		mp_grid_shader->AddStage(GL_VERTEX_SHADER, m_executable_directory + "/res/shaders/GridVS.glsl");
		mp_grid_shader->AddStage(GL_FRAGMENT_SHADER, m_executable_directory + "/res/shaders/GridFS.glsl");
		mp_grid_shader->Init();


		mp_quad_shader = &Renderer::GetShaderLibrary().CreateShader("2d_quad");
		mp_quad_shader->AddStage(GL_VERTEX_SHADER, m_executable_directory + "/../ORNG-Core/res/shaders/QuadVS.glsl");
		mp_quad_shader->AddStage(GL_FRAGMENT_SHADER, m_executable_directory + "/../ORNG-Core/res/shaders/QuadFS.glsl");
		mp_quad_shader->Init();

		mp_plane_shader = &Renderer::GetShaderLibrary().CreateShader("plane");
		mp_plane_shader->AddStage(GL_VERTEX_SHADER, m_executable_directory + "/res/shaders/QuadVS.glsl");
		mp_plane_shader->AddStage(GL_FRAGMENT_SHADER, m_executable_directory + "/res/shaders/PlaneFS.glsl");
		mp_plane_shader->Init();
		mp_plane_shader->AddUniform("u_plane");
		mp_plane_shader->AddUniform("u_col");


		mp_quad_col_shader = &Renderer::GetShaderLibrary().CreateShader("quad_col");
		mp_quad_col_shader->AddStage(GL_VERTEX_SHADER, m_executable_directory + "/../ORNG-Core/res/shaders/QuadVS.glsl", { "TRANSFORM" });
		mp_quad_col_shader->AddStage(GL_FRAGMENT_SHADER, m_executable_directory + "/../ORNG-Core/res/shaders/ColorFS.glsl");
		mp_quad_col_shader->Init();
		mp_quad_col_shader->AddUniform("u_scale");
		mp_quad_col_shader->AddUniform("u_translation");
		mp_quad_col_shader->AddUniform("u_color");

		mp_picking_shader = &Renderer::GetShaderLibrary().CreateShader("picking");
		mp_picking_shader->AddStage(GL_VERTEX_SHADER, m_executable_directory + "/../ORNG-Core/res/shaders/TransformVS.glsl");
		mp_picking_shader->AddStage(GL_FRAGMENT_SHADER, m_executable_directory + "/res/shaders/PickingFS.glsl");
		mp_picking_shader->Init();
		mp_picking_shader->AddUniform("comp_id");
		mp_picking_shader->AddUniform("transform");

		mp_highlight_shader = &Renderer::GetShaderLibrary().CreateShader("highlight");
		mp_highlight_shader->AddStage(GL_VERTEX_SHADER, m_executable_directory + "/../ORNG-Core/res/shaders/TransformVS.glsl");
		mp_highlight_shader->AddStage(GL_FRAGMENT_SHADER, m_executable_directory + "/res/shaders/ColorFS.glsl");
		mp_highlight_shader->Init();
		mp_highlight_shader->AddUniform("transform");
		mp_highlight_shader->AddUniform("u_color");

		// Setting up the scene display texture
		m_color_render_texture_spec.format = GL_RGBA;
		m_color_render_texture_spec.internal_format = GL_RGBA16F;
		m_color_render_texture_spec.storage_type = GL_FLOAT;
		m_color_render_texture_spec.mag_filter = GL_NEAREST;
		m_color_render_texture_spec.min_filter = GL_NEAREST;
		m_color_render_texture_spec.width = Window::GetWidth();
		m_color_render_texture_spec.height = Window::GetHeight();
		m_color_render_texture_spec.wrap_params = GL_CLAMP_TO_EDGE;

		mp_scene_display_texture = std::make_unique<Texture2D>("Editor scene display");
		mp_scene_display_texture->SetSpec(m_color_render_texture_spec);

		// Adding a resize event listener so the scene display texture scales with the window
		m_window_event_listener.OnEvent = [this](const Events::WindowEvent& t_event) {
			if (t_event.event_type == Events::Event::EventType::WINDOW_RESIZE) {
				auto spec = mp_scene_display_texture->GetSpec();
				spec.width = t_event.new_window_size.x;
				spec.height = t_event.new_window_size.y;
				mp_scene_display_texture->SetSpec(spec);
			}
			};

		Events::EventManager::RegisterListener(m_window_event_listener);

		Texture2DSpec picking_spec;
		picking_spec.format = GL_RG_INTEGER;
		picking_spec.internal_format = GL_RG32UI;
		picking_spec.storage_type = GL_UNSIGNED_INT;
		picking_spec.width = Window::GetWidth();
		picking_spec.height = Window::GetHeight();

		// Entity ID's are split into halves for storage in textures then recombined later as there is no format for 64 bit uints
		mp_picking_fb = &Renderer::GetFramebufferLibrary().CreateFramebuffer("picking", true);
		mp_picking_fb->AddRenderbuffer(Window::GetWidth(), Window::GetHeight());
		mp_picking_fb->Add2DTexture("component_ids_split", GL_COLOR_ATTACHMENT0, picking_spec);

		SceneRenderer::SetActiveScene(&*m_active_scene);
		static auto  s = &*m_active_scene;
		m_event_stack.SetContext(s);

		mp_editor_pass_fb = &Renderer::GetFramebufferLibrary().CreateFramebuffer("editor_passes", true);
		mp_editor_pass_fb->AddShared2DTexture("shared_depth", Renderer::GetFramebufferLibrary().GetFramebuffer("gbuffer").GetTexture<Texture2D>("shared_depth"), GL_DEPTH_ATTACHMENT);
		mp_editor_pass_fb->AddShared2DTexture("Editor scene display", *mp_scene_display_texture, GL_COLOR_ATTACHMENT0);


		std::string base_proj_dir = m_executable_directory + "\\projects\\base-project";
		if (!std::filesystem::exists(base_proj_dir)) {
			GenerateProject("base-project");
		}

		MakeProjectActive(base_proj_dir);

		ORNG_CORE_INFO("Editor layer initialized");
	}



	void EditorLayer::BeginPlayScene() {
		SceneSerializer::SerializeScene(*m_active_scene, m_temp_scene_serialization, true);
		m_simulate_mode_active = true;

		// Set to fullscreen so mouse coordinate and gui operations in scripts work correctly as they would in a runtime layer
		m_fullscreen_scene_display = true;
		m_active_scene->OnStart();
	}

	void EditorLayer::EndPlayScene() {
		glm::vec3 cam_pos = mp_editor_camera->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0];
		mp_editor_camera = nullptr;

		m_active_scene->ClearAllEntities();
		SceneSerializer::DeserializeScene(*m_active_scene, m_temp_scene_serialization, false, false);

		mp_editor_camera = std::make_unique<SceneEntity>(&*m_active_scene, m_active_scene->m_registry.create(), &m_active_scene->m_registry, m_active_scene->uuid());
		auto* p_transform = mp_editor_camera->AddComponent<TransformComponent>();
		p_transform->SetAbsolutePosition(cam_pos);
		mp_editor_camera->AddComponent<CameraComponent>()->MakeActive();

		m_simulate_mode_active = false;
		m_fullscreen_scene_display = false;
	}


	void EditorLayer::OnShutdown() {
		if (m_simulate_mode_active)
			EndPlayScene();

		mp_editor_camera = nullptr;
		m_active_scene->UnloadScene();
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


		mp_large_font = io.Fonts->AddFontFromFileTTF(".\\res\\fonts\\Uniform.ttf", 36.0f);

		ImGui_ImplOpenGL3_CreateFontsTexture();
		ImGui::StyleColorsDark();


		ImGui::GetStyle().Colors[ImGuiCol_Button] = lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_Border] = lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_Tab] = lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_TabHovered] = lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_TabActive] = orange_color;
		ImGui::GetStyle().Colors[ImGuiCol_Header] = lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_HeaderHovered] = lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_HeaderActive] = orange_color;
		ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_FrameBgHovered] = lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_FrameBgActive] = lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_DockingEmptyBg] = lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = dark_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_Border] = lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_TitleBg] = lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive] = lighter_grey_color;
	}



	void EditorLayer::Update() {
		ORNG_PROFILE_FUNC();

		if (Input::IsKeyDown('h')) {
			for (auto* p_ent : m_active_scene->m_entities) {
				if (p_ent->HasComponent<ParticleEmitterComponent>()) {
					p_ent->GetComponent<TransformComponent>()->SetPosition(p_ent->GetComponent<TransformComponent>()->GetPosition() + glm::vec3(100, 0, 0) * FrameTiming::GetTimeStep() * 0.001f);
				}
			}
		}

		if (Input::IsKeyDown('h')) {
			ORNG_CORE_TRACE("Key down");
		}

		if (m_fullscreen_scene_display)
			m_scene_display_rect = { ImVec2(Window::GetWidth(), Window::GetHeight() - toolbar_height) };
		else
			m_scene_display_rect = { ImVec2(Window::GetWidth() - (LEFT_WINDOW_WIDTH + RIGHT_WINDOW_WIDTH), Window::GetHeight() - m_asset_manager_window.window_height - toolbar_height) };


		if (Input::IsKeyDown(Key::LeftControl) && Input::IsKeyPressed('z') && !m_simulate_mode_active) { // Undo/redo disabled in simulation mode to prevent overlap during (de)serialization switching back and forth
			if (Input::IsKeyDown(Key::Shift))
				m_event_stack.Redo();
			else
				m_event_stack.Undo();
		}


		static bool dragging = false;
		if (Input::IsMouseClicked(0)) {
			m_mouse_drag_data.start = Input::GetMousePos();
			if (!m_fullscreen_scene_display) m_mouse_drag_data.start = ConvertFullscreenMouseToDisplayMouse(m_mouse_drag_data.start);
		}


		if (dragging) {
			m_mouse_drag_data.end = Input::GetMousePos();
			if (!m_fullscreen_scene_display) m_mouse_drag_data.end = ConvertFullscreenMouseToDisplayMouse(m_mouse_drag_data.end);
			MultiSelectDisplay();
		}

		dragging = ImGui::IsMouseDragging(0) && !ImGui::GetIO().WantCaptureMouse;

		if (Input::IsKeyDown(Key::K))
			mp_editor_camera->GetComponent<CameraComponent>()->MakeActive();

		if (Input::IsKeyDown(Key::F) && !m_selected_entity_ids.empty()) {
			auto* p_entity_transform = m_active_scene->GetEntity(m_selected_entity_ids[0])->GetComponent<TransformComponent>();
			auto* p_editor_transform = mp_editor_camera->GetComponent<TransformComponent>();
			auto pos = p_entity_transform->GetAbsoluteTransforms()[0];
			p_editor_transform->SetAbsolutePosition(pos - glm::vec3(5.0, 3.0, 5.0));
			p_editor_transform->LookAt(pos);
		}

		float ts = FrameTiming::GetTimeStep();
		UpdateEditorCam();

		if (m_simulate_mode_active) {
			mp_editor_camera->GetComponent<CameraComponent>()->aspect_ratio = (float)Window::GetWidth() / (float)Window::GetHeight();
			if (Input::IsKeyDown(Key::Escape))
				EndPlayScene();
		}

		else
			mp_editor_camera->GetComponent<CameraComponent>()->aspect_ratio = m_scene_display_rect.x / m_scene_display_rect.y;

		if (m_simulate_mode_active && !m_simulate_mode_paused)
			m_active_scene->Update(ts);
		else {
			m_active_scene->m_mesh_component_manager.OnUpdate(); // This still needs to update so meshes are rendered correctly in the editor
			m_active_scene->m_audio_system.OnUpdate(); // For accurate audio playback
			m_active_scene->terrain.UpdateTerrainQuadtree(m_active_scene->m_camera_system.GetActiveCamera()->GetEntity()->GetComponent<TransformComponent>()->GetPosition()); // Needed for terrain LOD updates
		}

		// Show/hide ui in simulation mode
		if (Input::IsKeyPressed(Key::Tab)) {
			m_render_ui = !m_render_ui;
		}


		// Duplicate entity keybind
		if (Input::IsKeyDown(Key::LeftControl) && Input::IsKeyPressed(Key::D)) {
			auto vec = DuplicateEntitiesTracked(m_selected_entity_ids);
			std::vector<uint64_t> duplicate_ids;
			for (auto* p_ent : vec) {
				duplicate_ids.push_back(p_ent->m_uuid());
			}
			m_selected_entity_ids = duplicate_ids;
		}
	}

	void EditorLayer::MultiSelectDisplay() {
		m_selected_entity_ids.clear();

		glm::vec2 min = { glm::min(m_mouse_drag_data.start.x,  m_mouse_drag_data.end.x), glm::min(Window::GetHeight() - m_mouse_drag_data.start.y,  Window::GetHeight() - m_mouse_drag_data.end.y) };
		glm::vec2 max = { glm::max(m_mouse_drag_data.start.x,  m_mouse_drag_data.end.x), glm::max(Window::GetHeight() - m_mouse_drag_data.start.y, Window::GetHeight() - m_mouse_drag_data.end.y) };
		glm::vec2 n = glm::vec2(m_scene_display_rect.x, m_scene_display_rect.y);


		auto* p_cam = m_active_scene->GetActiveCamera();
		auto* p_transform = p_cam->GetEntity()->GetComponent<TransformComponent>();
		auto pos = p_transform->GetAbsoluteTransforms()[0];

		glm::vec3 min_dir = ExtraMath::ScreenCoordsToRayDir(p_cam->GetProjectionMatrix(), min, pos, p_transform->forward, p_transform->up, Window::GetWidth(), Window::GetHeight());
		glm::vec3 max_dir = ExtraMath::ScreenCoordsToRayDir(p_cam->GetProjectionMatrix(), max, pos, p_transform->forward, p_transform->up, Window::GetWidth(), Window::GetHeight());

		glm::vec3 near_min = pos + min_dir * p_cam->zNear;

		glm::vec3 far_min = pos + min_dir * p_cam->zFar;

		glm::vec3 near_max = pos + max_dir * p_cam->zNear;

		glm::vec3 far_max = pos + max_dir * p_cam->zFar;


		float ratio = (float)glm::abs(m_mouse_drag_data.start.x - m_mouse_drag_data.end.x) / glm::abs(m_mouse_drag_data.start.y - m_mouse_drag_data.end.y);
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

		for (auto [entity, transform] : m_active_scene->m_registry.view<TransformComponent>().each()) {
			auto pos1 = transform.GetAbsoluteTransforms()[0];
			if (ne.GetSignedDistanceToPlane(pos1) >= 0 &&
				f.GetSignedDistanceToPlane(pos1) >= 0 &&
				r.GetSignedDistanceToPlane(pos1) >= 0 &&
				l.GetSignedDistanceToPlane(pos1) >= 0 &&
				t.GetSignedDistanceToPlane(pos1) >= 0 &&
				b.GetSignedDistanceToPlane(pos1) >= 0
				) {
				auto* p_entity = m_active_scene->GetEntity(entity);
				if (m_general_settings.selection_settings.select_only_parents && p_entity->GetParent() != entt::null)
					continue;

				if (!m_general_settings.selection_settings.select_all) {
					if (m_general_settings.selection_settings.select_light_objects && (p_entity->HasComponent<PointLightComponent>() || p_entity->HasComponent<SpotLightComponent>()))
						SelectEntity(m_active_scene->GetEntity(entity)->m_uuid());
					else if (m_general_settings.selection_settings.select_mesh_objects && p_entity->HasComponent<MeshComponent>())
						SelectEntity(m_active_scene->GetEntity(entity)->m_uuid());
					else if (m_general_settings.selection_settings.select_physics_objects && p_entity->HasComponent<PhysicsComponent>())
						SelectEntity(m_active_scene->GetEntity(entity)->m_uuid());
				}
				else {
					SelectEntity(m_active_scene->GetEntity(entity)->m_uuid());
				}
			}
		}
	}



	void EditorLayer::UpdateEditorCam() {
		static float cam_speed = 0.01f;

		auto* p_cam = mp_editor_camera->GetComponent<CameraComponent>();
		auto* p_transform = mp_editor_camera->GetComponent<TransformComponent>();
		auto abs_transforms = p_transform->GetAbsoluteTransforms();
		p_cam->aspect_ratio = m_scene_display_rect.x / m_scene_display_rect.y;
		// Camera movement
		if (ImGui::IsMouseDown(1)) {
			glm::vec3 pos = abs_transforms[0];
			glm::vec3 movement_vec{ 0.0, 0.0, 0.0 };
			float time_elapsed = FrameTiming::GetTimeStep();
			movement_vec += p_transform->right * (float)Input::IsKeyDown(GLFW_KEY_D) * time_elapsed * cam_speed;
			movement_vec -= p_transform->right * (float)Input::IsKeyDown(GLFW_KEY_A) * time_elapsed * cam_speed;
			movement_vec += p_transform->forward * (float)Input::IsKeyDown(Key::W) * time_elapsed * cam_speed;
			movement_vec -= p_transform->forward * (float)Input::IsKeyDown(GLFW_KEY_S) * time_elapsed * cam_speed;
			movement_vec += glm::vec3(0, 1, 0) * (float)Input::IsKeyDown(GLFW_KEY_E) * time_elapsed * cam_speed;
			movement_vec -= glm::vec3(0, 1, 0) * (float)Input::IsKeyDown(GLFW_KEY_Q) * time_elapsed * cam_speed;
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
				p_transform->LookAt(p_transform->GetAbsoluteTransforms()[0] + glm::normalize(rot_y));
			else
				p_transform->LookAt(p_transform->GetAbsoluteTransforms()[0] + glm::normalize(rot_x));


			Window::SetCursorPos(last_mouse_pos.x, last_mouse_pos.y);
		}

		p_cam->UpdateFrustum();
	}





	void EditorLayer::RenderSceneDisplayPanel() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::SetNextWindowPos(AddImVec2(ImGui::GetMainViewport()->Pos, ImVec2(LEFT_WINDOW_WIDTH, toolbar_height)));
		ImGui::SetNextWindowSize(m_scene_display_rect);


		if (ImGui::Begin("Scene display overlay", (bool*)0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | (ImGui::IsMouseDragging(0) ? 0 : ImGuiWindowFlags_NoInputs) | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground)) {
			ImVec2 prev_curs_pos = ImGui::GetCursorPos();
			ImGui::Image((ImTextureID)mp_scene_display_texture->GetTextureHandle(), ImVec2(m_scene_display_rect.x, m_scene_display_rect.y), ImVec2(0, 1), ImVec2(1, 0));
			ImGui::SetCursorPos(prev_curs_pos);
			ImGui::Dummy(ImVec2(0, 5));
			ImGui::Dummy(ImVec2(5, 0));
			ImGui::SameLine();
			ImGui::InvisibleButton("##drag-drop-scene-target", ImVec2(m_scene_display_rect.x - 20, m_scene_display_rect.y - 20));

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
						ent.GetComponent<TransformComponent>()->SetAbsolutePosition(p_cam_transform->GetAbsoluteTransforms()[0] + p_cam_transform->forward * (glm::max(glm::max(mesh_aabb.max.x, mesh_aabb.max.y), mesh_aabb.max.z) + 5.f));
						SelectEntity(ent.GetUUID());
					}
				}
				else if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("PREFAB")) {
					if (p_payload->DataSize == sizeof(Prefab*)) {
						std::string prefab_data = (*static_cast<Prefab**>(p_payload->Data))->serialized_content;
						auto& ent = m_simulate_mode_active ? m_active_scene->InstantiatePrefabCallScript(prefab_data) : m_active_scene->InstantiatePrefab(prefab_data);
						auto* p_cam_transform = mp_editor_camera->GetComponent<TransformComponent>();
						glm::vec3 pos;
						if (auto* p_mesh = ent.GetComponent<MeshComponent>()) {
							auto& mesh_aabb = p_mesh->GetMeshData()->m_aabb;
							pos = p_cam_transform->GetAbsoluteTransforms()[0] + p_cam_transform->forward * (glm::max(glm::max(mesh_aabb.max.x, mesh_aabb.max.y), mesh_aabb.max.z) + 5.f);
						}
						else {
							pos = p_cam_transform->GetAbsoluteTransforms()[0] + p_cam_transform->forward * 5.f;
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
		if (m_simulate_mode_active && !m_render_ui) {
			RenderToolbar();
			return;
		}

		if (!m_simulate_mode_active)
			RenderSceneDisplayPanel();


		RenderToolbar();
		m_asset_manager_window.OnRenderUI();


		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 5);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 5));
		ImGui::SetNextWindowPos(AddImVec2(ImGui::GetMainViewport()->Pos, ImVec2(0, toolbar_height)));
		ImGui::SetNextWindowSize(ImVec2(LEFT_WINDOW_WIDTH, Window::GetHeight() - toolbar_height - m_asset_manager_window.window_height));
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);

		ImGui::Begin("##left window", (bool*)0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar);
		ImGui::End();


		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		RenderSceneGraph();

		ImGui::SetNextWindowPos(AddImVec2(ImGui::GetMainViewport()->Pos, ImVec2(Window::GetWidth() - RIGHT_WINDOW_WIDTH, toolbar_height)));
		ImGui::SetNextWindowSize(ImVec2(RIGHT_WINDOW_WIDTH, Window::GetHeight() - toolbar_height));
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::Begin("##right window", (bool*)0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus)) {
			DisplayEntityEditor();
		}
		ImGui::End();

		RenderGeneralSettingsMenu();

		ImGui::PopStyleVar(); // window border size
		ImGui::PopStyleVar(); // window padding
	}




	void EditorLayer::RenderDisplayWindow() {
		Renderer::GetFramebufferLibrary().UnbindAllFramebuffers();
		GL_StateManager::DefaultClearBits();

		m_asset_manager_window.OnMainRender();

		if (ImGui::IsMouseClicked(0) && !ImGui::GetIO().WantCaptureMouse) {
			DoPickingPass();
		}


		SceneRenderer::SceneRenderingSettings settings;
		SceneRenderer::SetActiveScene(&*m_active_scene);
		settings.p_output_tex = &*mp_scene_display_texture;
		SceneRenderer::RenderScene(settings);

		mp_editor_pass_fb->Bind();
		DoSelectedEntityHighlightPass();
		RenderGrid();
		if (m_general_settings.debug_render_settings.render_physx_debug)
			RenderPhysxDebug();

		// Mouse drag selection quad
		if (ImGui::IsMouseDragging(0) && !ImGui::GetIO().WantCaptureMouse) {
			mp_quad_col_shader->ActivateProgram();
			mp_quad_col_shader->SetUniform("u_color", glm::vec4(0, 0, 1, 0.1));
			glm::vec2 w = { Window::GetWidth(), Window::GetHeight() };
			Renderer::DrawScaledQuad((glm::vec2(m_mouse_drag_data.start.x, Window::GetHeight() - m_mouse_drag_data.start.y) / w) * 2.f - 1.f, (glm::vec2(m_mouse_drag_data.end.x, Window::GetHeight() - m_mouse_drag_data.end.y) / w) * 2.f - 1.f);
		}

		Renderer::GetFramebufferLibrary().UnbindAllFramebuffers();

		if (m_simulate_mode_active) {
			mp_quad_shader->ActivateProgram();
			GL_StateManager::BindTexture(GL_TEXTURE_2D, mp_scene_display_texture->GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR);
			Renderer::DrawQuad();
		}
	}




	void EditorLayer::RenderGeneralSettingsMenu() {
		if (Input::IsKeyDown(Key::LeftControl) && Input::IsKeyPressed('j')) {
			m_render_settings_window = !m_render_settings_window;
		}

		if (!m_render_settings_window)
			return;

		if (ImGui::Begin("Settings")) {
			ImGui::SeparatorText("Debug rendering");
			ImGui::Checkbox("Render physx debug", &m_general_settings.debug_render_settings.render_physx_debug);

			ImGui::SeparatorText("Selection");
			ImGui::Checkbox("Select physics objects", &m_general_settings.selection_settings.select_physics_objects);
			ImGui::Checkbox("Select mesh objects", &m_general_settings.selection_settings.select_mesh_objects);
			ImGui::Checkbox("Select lights", &m_general_settings.selection_settings.select_light_objects);
			ImGui::Checkbox("Select all", &m_general_settings.selection_settings.select_all);
			ImGui::Checkbox("Select only parents", &m_general_settings.selection_settings.select_only_parents);
		}
		ImGui::End();
	}

	void EditorLayer::RenderToolbar() {
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
		ImGui::SetNextWindowSize(ImVec2(Window::GetWidth(), toolbar_height));

		if (ImGui::Begin("##Toolbar", (bool*)0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
			if (ImGui::Button("Files")) {
				ImGui::OpenPopup("FilePopup");
			}
			static std::vector<std::string> file_options{ "New project", "Open project", "Save project" };
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
				break;
			case 2: {
				//setting up file explorer callbacks
				wchar_t valid_extensions[MAX_PATH] = L"Project Files: *.yml\0*.yml\0";
				std::function<void(std::string)> success_callback = [this](std::string filepath) {
					MakeProjectActive(filepath.substr(0, filepath.find_last_of('\\')));
					};

				ExtraUI::ShowFileExplorer(m_executable_directory + "/projects", valid_extensions, success_callback);
				selected_component = 0;
				break;
			}
			case 3: {
				std::string scene_filepath{ "scene.yml" };
				std::string uuid_filepath{ "./res/scripts/includes/uuids.h" };
				AssetManager::SerializeAssets();
				SceneSerializer::SerializeScene(*m_active_scene, scene_filepath);
				SceneSerializer::SerializeSceneUUIDs(*m_active_scene, uuid_filepath);
				selected_component = 0;
			}
			}

			ImGui::SameLine();
			if (m_simulate_mode_active && ImGui::Button(ICON_FA_CIRCLE))
				EndPlayScene();
			else if (!m_simulate_mode_active && ImGui::Button(ICON_FA_PLAY))
				BeginPlayScene();

			ImGui::SameLine();
			if (m_simulate_mode_active && ((!m_simulate_mode_paused && ImGui::Button((ICON_FA_PAUSE)) || (m_simulate_mode_paused && ImGui::Button((ICON_FA_ANGLES_UP)))))) {
				m_simulate_mode_paused = !m_simulate_mode_paused;
			}


			ImGui::SameLine();

			if (ImGui::Button("Reload shaders")) {
				Renderer::GetShaderLibrary().ReloadShaders();
			}
			ImGui::SameLine();

			std::string sep_text = "Project: " + m_current_project_directory.substr(m_current_project_directory.find_last_of("\\") + 1);
			ImGui::SeparatorText(sep_text.c_str());
		}
		ImGui::End();
	}



	void EditorLayer::RenderProjectGenerator(int& selected_component_from_popup) {
		ImGui::SetNextWindowSize(ImVec2(500, 200));
		ImGui::SetNextWindowPos(ImVec2(Window::GetWidth() / 2 - 250, Window::GetHeight() / 2 - 100));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10);
		static std::string project_name;
		if (ImGui::Begin("##project gen", (bool*)0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
			if (ImGui::IsMouseDoubleClicked(1)) // close window
				selected_component_from_popup = 0;

			ImGui::PushFont(mp_large_font);
			ImGui::SeparatorText("Generate project");
			ImGui::PopFont();
			ImGui::Text("Name");
			ExtraUI::AlphaNumTextInput(project_name);

			static std::string err_msg = "";
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
			ImGui::TextWrapped(err_msg.c_str());
			ImGui::PopStyleColor();

			if (ImGui::Button("Generate")) {
				if (GenerateProject(project_name)) {
					MakeProjectActive(m_executable_directory + "\\projects\\" + project_name);
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
		FileDelete("./res/scripts/includes/SceneEntity.h");
		FileDelete("./res/scripts/includes/Scene.h");
		FileDelete("./res/scripts/includes/ScriptAPI.h");
		FileDelete("./res/scripts/includes/Component.h");
		FileDelete("./res/scripts/includes/MeshComponent.h");
		FileDelete("./res/scripts/includes/ScriptComponent.h");
		FileDelete("./res/scripts/includes/Lights.h");
		FileDelete("./res/scripts/includes/TransformComponent.h");
		FileDelete("./res/scripts/includes/PhysicsComponent.h");
		FileDelete("./res/scripts/includes/CameraComponent.h");
		FileDelete("./res/scripts/includes/EnttSingleInclude.h");
		FileDelete("./res/scripts/includes/SceneScriptInterface.h");
		FileDelete("./res/scripts/includes/AudioComponent.h");
		FileDelete("./res/scripts/includes/ScriptShared.h");
		FileDelete("./res/scripts/includes/ScriptInstancer.h");
		FileDelete("./res/scripts/includes/VehicleComponent.h");

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
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/scripting/SceneScriptInterface.h", "./res/scripts/includes/SceneScriptInterface.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/components/AudioComponent.h", "./res/scripts/includes/AudioComponent.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/scripting/ScriptShared.h", "./res/scripts/includes/ScriptShared.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/scripting/ScriptInstancer.h", "./res/scripts/includes/ScriptInstancer.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/components/VehicleComponent.h", "./res/scripts/includes/VehicleComponent.h");
	}


	bool EditorLayer::GenerateProject(const std::string& project_name) {
		if (std::filesystem::exists(m_executable_directory + "/projects/" + project_name)) {
			ORNG_CORE_ERROR("Project with name '{0}' already exists", project_name);
			return false;
		}

		if (!std::filesystem::exists(m_executable_directory + "/projects"))
			std::filesystem::create_directory(m_executable_directory + "/projects");

		std::string project_path = m_executable_directory + "/projects/" + project_name;
		std::filesystem::create_directory(project_path);
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

		RefreshScriptIncludes();

		/*// Include header files for API so intellisense works
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/scene/SceneEntity.h", project_path + "/res/scripts/includes/SceneEntity.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/extern/glm/glm/", project_path + "/res/scripts/includes/glm/", true);
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/scene/Scene.h", project_path + "/res/scripts/includes/Scene.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/scripting/ScriptAPI.h", project_path + "/res/scripts/includes/ScriptAPI.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/scripting/SceneScriptInterface.h", project_path + "/res/scripts/includes/SceneScriptInterface.h");
		// Components
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/components/Component.h", project_path + "/res/scripts/includes/Component.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/components/MeshComponent.h", project_path + "/res/scripts/includes/MeshComponent.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/components/ScriptComponent.h", project_path + "/res/scripts/includes/ScriptComponent.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/components/Lights.h", project_path + "/res/scripts/includes/Lights.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/components/TransformComponent.h", project_path + "/res/scripts/includes/TransformComponent.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/components/PhysicsComponent.h", project_path + "/res/scripts/includes/PhysicsComponent.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/components/CameraComponent.h", project_path + "/res/scripts/includes/CameraComponent.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/headers/components/AudioComponent.h", project_path + "/res/scripts/includes/AudioComponent.h");
		FileCopy(ORNG_CORE_MAIN_DIR "/extern/entt/EnttSingleInclude.h", project_path + "/res/scripts/includes/EnttSingleInclude.h");*/

		return true;
	}




	bool EditorLayer::ValidateProjectDir(const std::string& dir_path) {
		try {
			if (!std::filesystem::exists(dir_path + "/scene.yml")) {
				std::ofstream s{ dir_path + "/scene.yml" };
				s << ORNG_BASE_SCENE_YAML;
				s.close();
			}

			if (!std::filesystem::exists(dir_path + "/res/"))
				std::filesystem::create_directory(dir_path + "/res");

			if (!std::filesystem::exists(dir_path + "/res/meshes"))
				std::filesystem::create_directory(dir_path + "/res/meshes");

			if (!std::filesystem::exists(dir_path + "/res/textures"))
				std::filesystem::create_directory(dir_path + "/res/textures");

			if (!std::filesystem::exists(dir_path + "/res/shaders"))
				std::filesystem::create_directory(dir_path + "/res/shaders");

			if (!std::filesystem::exists(dir_path + "/res/scripts"))
				std::filesystem::create_directory(dir_path + "/res/scripts");

			if (!std::filesystem::exists(dir_path + "/res/prefabs"))
				std::filesystem::create_directory(dir_path + "/res/prefabs");

			if (!std::filesystem::exists(dir_path + "/res/materials"))
				std::filesystem::create_directory(dir_path + "/res/materials");

			if (!std::filesystem::exists(dir_path + "/res/audio"))
				std::filesystem::create_directory(dir_path + "/res/audio");

			if (!std::filesystem::exists(dir_path + "/res/physx-materials"))
				std::filesystem::create_directory(dir_path + "/res/physx-materials");



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
		if (ValidateProjectDir(folder_path)) {
			if (m_simulate_mode_active)
				EndPlayScene();

			std::filesystem::current_path(folder_path);
			m_current_project_directory = folder_path;
			RefreshScriptIncludes();

			// Deselect material and texture that's about to be deleted
			m_asset_manager_window.SelectMaterial(nullptr);
			m_asset_manager_window.SelectTexture(nullptr);

			m_selected_entity_ids.clear();

			glm::vec3 cam_pos = mp_editor_camera ? mp_editor_camera->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0] : glm::vec3{ 0, 0, 0 };
			mp_editor_camera = nullptr; // Delete explicitly here to properly remove it from the scene before unloading


			if (m_active_scene->m_is_loaded)
				m_active_scene->UnloadScene();

			AssetManager::ClearAll();
			AssetManager::LoadAssetsFromProjectPath(m_current_project_directory, false);
			m_active_scene->LoadScene(m_current_project_directory + "\\scene.yml");
			SceneSerializer::DeserializeScene(*m_active_scene, m_current_project_directory + "\\scene.yml", true);


			mp_editor_camera = std::make_unique<SceneEntity>(&*m_active_scene, m_active_scene->m_registry.create(), &m_active_scene->m_registry, m_active_scene->uuid());
			auto* p_transform = mp_editor_camera->AddComponent<TransformComponent>();
			p_transform->SetAbsolutePosition(cam_pos);
			mp_editor_camera->AddComponent<CameraComponent>()->MakeActive();

			for (int i = 0; i < 6; i++) {
				auto& ent = m_active_scene->CreateEntity("e");
				ent.GetComponent<TransformComponent>()->SetPosition(i * 20, 0, 750);
				ent.AddComponent<ParticleEmitterComponent>();
			}
		}
		else {
			ORNG_CORE_ERROR("Project folder path invalid");
			return false;
		}
		return true;
	}



	void EditorLayer::RenderCreationWidget(SceneEntity* p_entity, bool trigger) {
		const char* names[9] = { "Pointlight", "Spotlight", "Mesh", "Camera", "Physics", "Script", "Audio", "Vehicle", "Particle emitter" };

		if (trigger)
			ImGui::OpenPopup("my_select_popup");

		int selected_component = -1;
		if (ImGui::BeginPopup("my_select_popup"))
		{
			ImGui::SeparatorText("Create entity");
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
		}
	}



	glm::vec2 EditorLayer::ConvertFullscreenMouseToDisplayMouse(glm::vec2 mouse_coords) {
		// Transform mouse coordinates to full window space for the proper texture coordinates
		mouse_coords.x -= LEFT_WINDOW_WIDTH;
		mouse_coords.x *= (Window::GetWidth() / ((float)Window::GetWidth() - (RIGHT_WINDOW_WIDTH + LEFT_WINDOW_WIDTH)));
		mouse_coords.y -= toolbar_height;
		mouse_coords.y *= (float)Window::GetHeight() / ((float)Window::GetHeight() - m_asset_manager_window.window_height - toolbar_height);
		return mouse_coords;
	}


	void EditorLayer::DoPickingPass() {
		mp_picking_fb->Bind();
		mp_picking_shader->ActivateProgram();

		GL_StateManager::ClearDepthBits();
		GL_StateManager::ClearBitsUnsignedInt();

		auto view = m_active_scene->m_registry.view<MeshComponent>();
		for (auto [entity, mesh] : view.each()) {
			//Split uint64 into two uint32's for texture storage
			uint64_t full_id = mesh.GetEntityUUID();
			uint32_t half_id_1 = (uint32_t)(full_id >> 32);
			uint32_t half_id_2 = (uint32_t)(full_id);
			glm::uvec2 id_vec{ half_id_1, half_id_2 };

			mp_picking_shader->SetUniform("comp_id", id_vec);
			mp_picking_shader->SetUniform("transform", mesh.GetEntity()->GetComponent<TransformComponent>()->GetMatrix());

			Renderer::DrawMeshInstanced(mesh.GetMeshData(), 1);
		}

		glm::vec2 mouse_coords = glm::min(glm::max(glm::vec2(Input::GetMousePos()), glm::vec2(1, 1)), glm::vec2(Window::GetWidth() - 1, Window::GetHeight() - 1));

		if (!m_fullscreen_scene_display) {
			mouse_coords = ConvertFullscreenMouseToDisplayMouse(mouse_coords);
		}

		uint32_t* pixels = new uint32_t[2];
		glReadPixels(mouse_coords.x, Window::GetHeight() - mouse_coords.y, 1, 1, GL_RG_INTEGER, GL_UNSIGNED_INT, pixels);
		uint64_t current_entity_id = ((uint64_t)pixels[0] << 32) | pixels[1];
		delete[] pixels;



		if (!Input::IsKeyDown(GLFW_KEY_LEFT_CONTROL))
			m_selected_entity_ids.clear();

		SelectEntity(current_entity_id);
	}




	void EditorLayer::DoSelectedEntityHighlightPass() {
		glDisable(GL_DEPTH_TEST);
		mp_editor_pass_fb->Bind();
		mp_highlight_shader->ActivateProgram();
		mp_highlight_shader->SetUniform("u_color", glm::vec4(1.0, 0, 0, 0.1));

		for (auto id : m_selected_entity_ids) {
			auto* current_entity = m_active_scene->GetEntity(id);

			if (!current_entity || !current_entity->HasComponent<MeshComponent>())
				continue;

			MeshComponent* meshc = current_entity->GetComponent<MeshComponent>();


			mp_highlight_shader->SetUniform("transform", meshc->GetEntity()->GetComponent<TransformComponent>()->GetMatrix());
			Renderer::DrawMeshInstanced(meshc->GetMeshData(), 1);
		}
		glEnable(GL_DEPTH_TEST);
	}




	void EditorLayer::RenderGrid() {
		GL_StateManager::BindSSBO(m_grid_mesh->m_ssbo_handle, GL_StateManager::SSBO_BindingPoints::TRANSFORMS);
		m_grid_mesh->CheckBoundary(mp_editor_camera->GetComponent<TransformComponent>()->GetPosition());
		mp_grid_shader->ActivateProgram();
		Renderer::DrawVAO_ArraysInstanced(GL_LINES, m_grid_mesh->m_vao, ceil(m_grid_mesh->grid_width / m_grid_mesh->grid_step) * 2);
	}

	void EditorLayer::RenderPhysxDebug() {
		mp_highlight_shader->ActivateProgram();
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		mp_highlight_shader->SetUniform("u_color", glm::vec4(0.0, 1.0, 0, 0.1));
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		// Break into 3 sep. loops to reduce vao changes
		for (auto [entity, phys, transform] : m_active_scene->m_registry.view<PhysicsComponent, TransformComponent>().each()) {
			if (phys.m_geometry_type == PhysicsComponent::BOX) {
				mp_highlight_shader->SetUniform("transform", transform.GetMatrix());
				Renderer::DrawCube();
			}
		}

		for (auto [entity, phys, transform] : m_active_scene->m_registry.view<PhysicsComponent, TransformComponent>().each()) {
			if (phys.m_geometry_type == PhysicsComponent::SPHERE) {
				auto at = transform.GetAbsoluteTransforms();
				auto sf = ((PxSphereGeometry*)&phys.p_shape->getGeometry())->radius;

				auto s = at[1];
				auto t = at[0];
				auto r = at[2];
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
					m = transform.GetParent()->GetMatrix() * glm::inverse(ExtraMath::Init3DScaleTransform(s.x, s.y, s.z)) * (trans_mat * rot_mat * scale_mat);
				}

				// Undo scaling to prevent shearing
				mp_highlight_shader->SetUniform("transform", m);
				Renderer::DrawSphere();
			}
		}

		for (auto [entity, mesh, phys, transform] : m_active_scene->m_registry.view<MeshComponent, PhysicsComponent, TransformComponent>().each()) {
			if (phys.m_geometry_type == PhysicsComponent::TRIANGLE_MESH) {
				mp_highlight_shader->SetUniform("transform", transform.GetMatrix());
				for (int i = 0; i < mesh.mp_mesh_asset->m_submeshes.size(); i++) {
					Renderer::DrawSubMesh(mesh.mp_mesh_asset, i);
				}
			}
		}


		glEnable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}




	void EditorLayer::RenderSkyboxEditor() {
		if (ExtraUI::H1TreeNode("Skybox")) {
			wchar_t valid_extensions[MAX_PATH] = L"Texture Files: *.png;*.jpg;*.jpeg;*.hdr\0*.png;*.jpg;*.jpeg;*.hdr\0";

			std::function<void(std::string)> file_explorer_callback = [this](std::string filepath) {
				// Check if texture is an asset or not, if not, add it
				std::string new_filepath = "./res/textures/" + filepath.substr(filepath.find_last_of("\\") + 1);
				if (!std::filesystem::exists(new_filepath)) {
					FileCopy(filepath, new_filepath);
				}
				m_active_scene->skybox.LoadEnvironmentMap(new_filepath);
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
		}
	}




	EditorLayer::EntityNodeEvent EditorLayer::RenderEntityNode(SceneEntity* p_entity, unsigned int layer) {
		EntityNodeEvent ret = EntityNodeEvent::E_NONE;

		// Tree nodes that are open are stored here so their children are rendered with the logic below, independant of if the parent tree node is visible or not.
		static std::vector<uint64_t> open_tree_nodes;

		static std::string padding_str;
		for (int i = 0; i < layer; i++) {
			padding_str += "|--";
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
			formatted_name += " ";
			formatted_name += p_entity->name;
		}

		auto* p_entity_relationship_comp = p_entity->GetComponent<RelationshipComponent>();

		ImVec4 tree_node_bg_col = VectorContains(m_selected_entity_ids, p_entity->GetUUID()) ? lighter_grey_color : ImVec4(0, 0, 0, 0);
		ImGui::PushStyleColor(ImGuiCol_Header, tree_node_bg_col);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 3));
		ImGui::PushID(p_entity);


		auto flags = p_entity_relationship_comp->first == entt::null ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Framed : ImGuiTreeNodeFlags_Framed;
		flags |= p_entity_relationship_comp->num_children > 0 ? ImGuiTreeNodeFlags_OpenOnArrow : 0;
		bool is_tree_node_open = ImGui::TreeNodeEx(formatted_name.c_str(), flags);

		if (ImGui::IsItemToggledOpen()) {
			auto it = std::ranges::find(open_tree_nodes, p_entity->GetUUID());
			if (it == open_tree_nodes.end())
				open_tree_nodes.push_back(p_entity->GetUUID());
			else
				open_tree_nodes.erase(it);
		}

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("ENTITY")) {
				/* Functionality handled externally */
			}
			ImGui::EndDragDropTarget();
		}

		m_selected_entities_are_dragged |= ImGui::IsItemHovered() && ImGui::IsMouseDragging(0);

		if (m_selected_entities_are_dragged) {
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceExtern)) {
				ImGui::SetDragDropPayload("ENTITY", &m_selected_entity_ids, sizeof(std::vector<uint64_t>));
				ImGui::EndDragDropSource();
			}
		}

		// Drag entities into another entity node to make them children of it
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && m_selected_entities_are_dragged && ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax())) {
			DeselectEntity(p_entity->GetUUID());
			for (auto id : m_selected_entity_ids) {
				m_active_scene->GetEntity(id)->SetParent(*p_entity);
			}

			m_selected_entities_are_dragged = false;
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
					m_selected_entity_ids.clear();

				if (Input::IsKeyDown(GLFW_KEY_LEFT_CONTROL) && VectorContains(m_selected_entity_ids, p_entity->GetUUID()) && ImGui::IsMouseClicked(0)) // Deselect entity from group of entities currently selected
					m_selected_entity_ids.erase(std::ranges::find(m_selected_entity_ids, p_entity->GetUUID()));
				else
					SelectEntity(p_entity->GetUUID());
			}
		}

		// Popup opened above if node is right clicked
		if (ImGui::BeginPopup(popup_id.c_str()))
		{
			ImGui::SeparatorText("Options");
			if (ImGui::Selectable("Delete"))
				ret = EntityNodeEvent::E_DELETE;

			// Creates clone of entity, puts it in scene
			if (ImGui::Selectable("Duplicate"))
				ret = EntityNodeEvent::E_DUPLICATE;

			ImGui::EndPopup();
		}

		if (is_tree_node_open) {
			ImGui::TreePop(); // Pop tree node opened earlier
		}
		// Render entity nodes for all the children of this entity
		if (p_entity && VectorContains(open_tree_nodes, p_entity->GetUUID())) {
			entt::entity current_child_entity = p_entity_relationship_comp->first;
			while (current_child_entity != entt::null) {
				auto& child_rel_comp = m_active_scene->m_registry.get<RelationshipComponent>(current_child_entity);
				// Render child entity node, propagate event up
				ret = static_cast<EntityNodeEvent>((ret | RenderEntityNode(child_rel_comp.GetEntity(), layer + 1)));
				current_child_entity = child_rel_comp.next;
			}
		}

		ImGui::PopID();
		return ret;
	}



	void EditorLayer::RenderSceneGraph() {
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::Begin("Scene graph")) {
			// Click anywhere on window to deselect entity nodes
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !Input::IsKeyDown(GLFW_KEY_LEFT_CONTROL))
				m_selected_entity_ids.clear();

			// Right click to bring up "new entity" popup
			RenderCreationWidget(nullptr, ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1));

			ImGui::Text("Editor cam exposure");
			ImGui::SliderFloat("##exposure", &mp_editor_camera->GetComponent<CameraComponent>()->exposure, 0.f, 10.f);

			EntityNodeEvent active_event = EntityNodeEvent::E_NONE;


			if (ExtraUI::H2TreeNode("Entities")) {
				m_general_settings.editor_window_settings.display_skybox_editor = ExtraUI::EmptyTreeNode("Skybox");
				m_general_settings.editor_window_settings.display_directional_light_editor = ExtraUI::EmptyTreeNode("Directional Light");
				m_general_settings.editor_window_settings.display_global_fog_editor = ExtraUI::EmptyTreeNode("Global fog");
				m_general_settings.editor_window_settings.display_terrain_editor = ExtraUI::EmptyTreeNode("Terrain");
				m_general_settings.editor_window_settings.display_bloom_editor = ExtraUI::EmptyTreeNode("Bloom");

				for (auto* p_entity : m_active_scene->m_entities) {
					ASSERT(p_entity->HasComponent<RelationshipComponent>());
					ASSERT(p_entity->HasComponent<TransformComponent>());
					if (p_entity->GetComponent<RelationshipComponent>()->parent != entt::null)
						continue;

					active_event = (EntityNodeEvent)(RenderEntityNode(p_entity, 0) | active_event);
				}
			}

			// Process node events
			if (active_event & EntityNodeEvent::E_DUPLICATE) {
				DuplicateEntitiesTracked(m_selected_entity_ids);
			}
			else if (active_event & EntityNodeEvent::E_DELETE) {
				for (auto id : m_selected_entity_ids) {
					if (auto* p_entity = m_active_scene->GetEntity(id))
						DeleteEntitiesTracked(m_selected_entity_ids);
				}
			}


			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
				if (m_selected_entities_are_dragged && ImGui::IsWindowHovered()) {
					for (auto id : m_selected_entity_ids) {
						m_active_scene->GetEntity(id)->RemoveParent();
					}
				}

				m_selected_entities_are_dragged = false;
			}
		} // begin "scene graph"

		ImGui::End();
	}




	void EditorLayer::DisplayEntityEditor() {
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::Begin("Properties")) {
			if (m_general_settings.editor_window_settings.display_directional_light_editor)
				RenderDirectionalLightEditor();
			if (m_general_settings.editor_window_settings.display_global_fog_editor)
				RenderGlobalFogEditor();
			if (m_general_settings.editor_window_settings.display_skybox_editor)
				RenderSkyboxEditor();
			if (m_general_settings.editor_window_settings.display_terrain_editor)
				RenderTerrainEditor();
			if (m_general_settings.editor_window_settings.display_bloom_editor)
				RenderBloomEditor();

			auto entity = m_active_scene->GetEntity(m_selected_entity_ids.empty() ? 0 : m_selected_entity_ids[0]);
			if (!entity) {
				ImGui::End();
				return;
			}

			auto meshc = entity->GetComponent<MeshComponent>();
			auto plight = entity->GetComponent<PointLightComponent>();
			auto slight = entity->GetComponent<SpotLightComponent>();
			auto p_cam = entity->GetComponent<CameraComponent>();
			PhysicsComponent* p_physics_comp = entity->GetComponent<PhysicsComponent>();
			auto* p_script_comp = entity->GetComponent<ScriptComponent>();
			auto* p_audio_comp = entity->GetComponent<AudioComponent>();
			auto* p_vehicle_comp = entity->GetComponent<VehicleComponent>();
			auto* p_emitter_comp = entity->GetComponent<ParticleEmitterComponent>();

			static std::vector<TransformComponent*> transforms;
			transforms.clear();
			for (auto id : m_selected_entity_ids) {
				transforms.push_back(m_active_scene->GetEntity(id)->GetComponent<TransformComponent>());
			}
			ImGui::Text("Name: ");
			ImGui::SameLine();
			ExtraUI::AlphaNumTextInput(entity->name);


			//TRANSFORM
			if (ExtraUI::H2TreeNode("Entity transform")) {
				RenderTransformComponentEditor(transforms);
			}


			//MESH
			if (meshc) {
				if (ExtraUI::H2TreeNode("Mesh component")) {
					if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(1)) {
						entity->DeleteComponent<MeshComponent>();
					}
					if (meshc->mp_mesh_asset) {
						RenderMeshComponentEditor(meshc);
					}
				}
			}


			ImGui::PushID(plight);
			if (plight && ExtraUI::H2TreeNode("Pointlight component")) {
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(1)) {
					entity->DeleteComponent<PointLightComponent>();
				}
				else {
					RenderPointlightEditor(plight);
				}
			}
			ImGui::PopID();

			ImGui::PushID(slight);
			if (slight && ExtraUI::H2TreeNode("Spotlight component")) {
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(1)) {
					entity->DeleteComponent<SpotLightComponent>();
				}
				else {
					RenderSpotlightEditor(slight);
				}
			}
			ImGui::PopID();

			ImGui::PushID(p_cam);
			if (p_cam && ExtraUI::H2TreeNode("Camera component")) {
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(1)) {
					entity->DeleteComponent<CameraComponent>();
				}
				RenderCameraEditor(p_cam);
			}
			ImGui::PopID();

			ImGui::PushID(p_physics_comp);
			if (p_physics_comp && ExtraUI::H2TreeNode("Physics component")) {
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(1)) {
					entity->DeleteComponent<PhysicsComponent>();
				}
				else {
					RenderPhysicsComponentEditor(p_physics_comp);
				}
			}
			ImGui::PopID();

			if (p_script_comp && ExtraUI::H2TreeNode("Script component")) {
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(1)) {
					entity->DeleteComponent<ScriptComponent>();
				}
				else {
					RenderScriptComponentEditor(p_script_comp);
				}
			}

			if (p_audio_comp && ExtraUI::H2TreeNode("Audio component")) {
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(1)) {
					entity->DeleteComponent<AudioComponent>();
				}
				else {
					RenderAudioComponentEditor(p_audio_comp);
				}
			}

			if (p_vehicle_comp && ExtraUI::H2TreeNode("Vehicle component")) {
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(1)) {
					entity->DeleteComponent<VehicleComponent>();
				}
				else {
					RenderVehicleComponentEditor(p_vehicle_comp);
				}
			}

			if (p_emitter_comp && ExtraUI::H2TreeNode("Particle emitter component")) {
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(1)) {
					entity->DeleteComponent<ParticleEmitterComponent>();
				}
				else {
					RenderParticleEmitterComponentEditor(p_emitter_comp);
				}
			}

			glm::vec2 window_size = { ImGui::GetWindowSize().x, ImGui::GetWindowSize().y };
			glm::vec2 button_size = { 200, 50 };
			glm::vec2 padding_size = { (window_size.x / 2.f) - button_size.x / 2.f, 50.f };
			ImGui::Dummy(ImVec2(padding_size.x, padding_size.y));

			RenderCreationWidget(entity, ImGui::Button("Add component", ImVec2(button_size.x, button_size.y)));
		}
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
				if (p_payload->DataSize == sizeof(std::string)) {
					ScriptSymbols* p_symbols = &AssetManager::GetAsset<ScriptAsset>(*static_cast<std::string*>(p_payload->Data))->symbols;
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
				m_active_scene->m_physics_system.InitVehicle(p_comp);
			}
			ImGui::PopID();
		}
	}

	void EditorLayer::RenderParticleEmitterComponentEditor(ParticleEmitterComponent* p_comp) {
		ImGui::PushID(p_comp);


		std::function<void(MeshAsset* p_new)> OnMeshDrop = [p_comp](MeshAsset* p_new) {
			p_comp->p_particle_mesh = p_new;
			p_comp->materials = { p_new->GetNbMaterials(), AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID)};
		};

		std::function<void(unsigned index, Material* p_new)> OnMaterialDrop = [p_comp](unsigned index, Material* p_new) {
			p_comp->materials[index] = p_new;
		};

		RenderMeshWithMaterials(p_comp->p_particle_mesh, p_comp->materials, OnMeshDrop, OnMaterialDrop);


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
		ExtraUI::ColoredButton("##playback position", orange_color_dark, ImVec2(playback_widget_width * ((float)position / (float)total_length), playback_widget_height));
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

				p_audio->mp_channel->setPosition(((float)local_mouse.x / playback_widget_width) * (float)total_length, FMOD_TIMEUNIT_MS);
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
		if (m_fullscreen_scene_display)
			ImGuizmo::SetRect(ImGui::GetMainViewport()->Pos.x, ImGui::GetMainViewport()->Pos.y + toolbar_height, m_scene_display_rect.x, m_scene_display_rect.y);
		else
			ImGuizmo::SetRect(ImGui::GetMainViewport()->Pos.x + LEFT_WINDOW_WIDTH, ImGui::GetMainViewport()->Pos.y + toolbar_height, m_scene_display_rect.x, m_scene_display_rect.y);

		static ImGuizmo::OPERATION current_operation = ImGuizmo::TRANSLATE;
		static ImGuizmo::MODE current_mode = ImGuizmo::WORLD;


		if (Input::IsKeyDown(GLFW_KEY_1))
			current_operation = ImGuizmo::TRANSLATE;
		else if (Input::IsKeyDown(GLFW_KEY_2))
			current_operation = ImGuizmo::SCALE;
		else if (Input::IsKeyDown(GLFW_KEY_3))
			current_operation = ImGuizmo::ROTATE;

		ImGui::Text("Gizmo rendering");
		if (ImGui::RadioButton("World", current_mode == ImGuizmo::WORLD))
			current_mode = ImGuizmo::WORLD;
		ImGui::SameLine();
		if (ImGui::RadioButton("Local", current_mode == ImGuizmo::LOCAL))
			current_mode = ImGuizmo::LOCAL;

		glm::mat4 current_operation_matrix = transforms[0]->GetMatrix();

		CameraComponent* p_cam = m_active_scene->m_camera_system.GetActiveCamera();
		auto* p_cam_transform = p_cam->GetEntity()->GetComponent<TransformComponent>();
		glm::vec3 pos = p_cam_transform->GetAbsoluteTransforms()[0];
		glm::mat4 view_mat = glm::lookAt(pos, pos + p_cam_transform->forward, p_cam_transform->up);

		glm::mat4 delta_matrix;
		ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
		static glm::vec3 snap = glm::vec3(0.01f);

		static bool is_using = false;
		static bool mouse_down = false;



		if (ImGuizmo::Manipulate(&view_mat[0][0], &p_cam->GetProjectionMatrix()[0][0], current_operation, current_mode, &current_operation_matrix[0][0], &delta_matrix[0][0], &snap[0]) && ImGuizmo::IsUsing()) {
			if (!is_using && !mouse_down && !m_simulate_mode_active) {
				EditorEvent e;
				e.event_type = TRANSFORM_UPDATE;
				e.affected_entities = m_selected_entity_ids;
				for (auto id : m_selected_entity_ids) {
					e.serialized_entities_before.push_back(SceneSerializer::SerializeEntityIntoString(*m_active_scene->GetEntity(id)));
				}
				m_event_stack.PushEvent(e);
			}



			ImGuizmo::DecomposeMatrixToComponents(&delta_matrix[0][0], &matrix_translation[0], &matrix_rotation[0], &matrix_scale[0]);

			auto base_abs_transforms = transforms[0]->GetAbsoluteTransforms();
			glm::vec3 base_abs_translation = base_abs_transforms[0];
			glm::vec3 base_abs_scale = base_abs_transforms[1];
			glm::vec3 base_abs_rotation = base_abs_transforms[2];

			glm::vec3 delta_translation = matrix_translation;
			glm::vec3 delta_scale = matrix_scale;
			glm::vec3 delta_rotation = matrix_rotation;

			static std::vector<glm::vec3> scale_dividers;

			for (auto* p_transform : transforms) {
				switch (current_operation) {
				case ImGuizmo::TRANSLATE:
					p_transform->SetAbsolutePosition(p_transform->GetAbsoluteTransforms()[0] + delta_translation);
					break;
				case ImGuizmo::SCALE:
				{
					p_transform->SetScale(p_transform->m_scale * delta_scale);
					break;
				}
				case ImGuizmo::ROTATE: // This will rotate multiple objects as one, using entity transform at m_selected_entity_ids[0] as origin
					auto current_transforms = p_transform->GetAbsoluteTransforms();
					if (auto* p_parent_transform = p_transform->GetParent()) {
						glm::vec3 s = p_parent_transform->GetAbsoluteTransforms()[1];
						glm::mat3 to_parent_space = p_parent_transform->GetMatrix() * glm::inverse(ExtraMath::Init3DScaleTransform(s.x, s.y, s.z));
						glm::vec3 local_rot = glm::inverse(to_parent_space) * glm::vec4(delta_rotation, 0.0);
						glm::vec3 total = glm::eulerAngles(glm::quat(glm::radians(local_rot)) * glm::quat(glm::radians(p_transform->m_orientation)));
						p_transform->SetOrientation(glm::degrees(total));
					}
					else {
						auto orientation = glm::degrees(glm::eulerAngles(glm::quat(glm::radians(delta_rotation)) * glm::quat(glm::radians(p_transform->m_orientation))));
						p_transform->SetOrientation(orientation.x, orientation.y, orientation.z);
					}

					glm::vec3 abs_translation = current_transforms[0];
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




	bool EditorLayer::RenderMeshWithMaterials(const MeshAsset* p_asset,  std::vector<const Material*>& materials, std::function<void(MeshAsset* p_new)> OnMeshDrop, std::function<void(unsigned index, Material* p_new)> OnMaterialDrop) {
		ImGui::PushID(p_asset);

		bool ret = false;

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
			ImGui::SliderFloat("##scattering", &m_active_scene->post_processing.global_fog.scattering_coef, 0.f, 0.1f);
			ImGui::Text("Absorption");
			ImGui::SliderFloat("##absorption", &m_active_scene->post_processing.global_fog.absorption_coef, 0.f, 0.1f);
			ImGui::Text("Density");
			ImGui::SliderFloat("##density", &m_active_scene->post_processing.global_fog.density_coef, 0.f, 1.f);
			ImGui::Text("Scattering anistropy");
			ImGui::SliderFloat("##scattering anistropy", &m_active_scene->post_processing.global_fog.scattering_anistropy, -1.f, 1.f);
			ImGui::Text("Emissive factor");
			ImGui::SliderFloat("##emissive", &m_active_scene->post_processing.global_fog.emissive_factor, 0.f, 2.f);
			ImGui::Text("Step count");
			ImGui::SliderInt("##step count", &m_active_scene->post_processing.global_fog.step_count, 0, 512);
			ExtraUI::ShowColorVec3Editor("Color", m_active_scene->post_processing.global_fog.color);
		}
	}




	void EditorLayer::RenderBloomEditor() {
		if (ExtraUI::H2TreeNode("Bloom")) {
			ImGui::SliderFloat("Intensity", &m_active_scene->post_processing.bloom.intensity, 0.f, 10.f);
			ImGui::SliderFloat("Threshold", &m_active_scene->post_processing.bloom.threshold, 0.0f, 50.0f);
			ImGui::SliderFloat("Knee", &m_active_scene->post_processing.bloom.knee, 0.f, 1.f);
		}
	}




	void EditorLayer::RenderTerrainEditor() {
		if (ExtraUI::H2TreeNode("Terrain")) {
			ImGui::InputFloat("Height factor", &m_active_scene->terrain.m_height_scale);
			static int terrain_width = 1000;

			if (ImGui::InputInt("Size", &terrain_width) && terrain_width > 500)
				m_active_scene->terrain.m_width = terrain_width;
			else
				terrain_width = m_active_scene->terrain.m_width;

			static int terrain_seed = 123;
			ImGui::InputInt("Seed", &terrain_seed);
			m_active_scene->terrain.m_seed = terrain_seed;

			if (auto* p_new_material = RenderMaterialComponent(m_active_scene->terrain.mp_material))
				m_active_scene->terrain.mp_material = p_new_material;

			if (ImGui::Button("Reload"))
				m_active_scene->terrain.ResetTerrainQuadtree();
		}
	}




	void EditorLayer::RenderDirectionalLightEditor() {
		if (ExtraUI::H1TreeNode("Directional light")) {
			ImGui::Text("DIR LIGHT CONTROLS");

			static glm::vec3 light_dir = glm::vec3(0, 0.5, 0.5);
			static glm::vec3 light_color = m_active_scene->directional_light.color;

			ImGui::SliderFloat("X", &light_dir.x, -1.f, 1.f);
			ImGui::SliderFloat("Y", &light_dir.y, -1.f, 1.f);
			ImGui::SliderFloat("Z", &light_dir.z, -1.f, 1.f);
			ExtraUI::ShowColorVec3Editor("Color", light_color);

			ImGui::Text("Cascade ranges");
			ImGui::SliderFloat("##c1", &m_active_scene->directional_light.cascade_ranges[0], 0.f, 50.f);
			ImGui::SliderFloat("##c2", &m_active_scene->directional_light.cascade_ranges[1], 0.f, 150.f);
			ImGui::SliderFloat("##c3", &m_active_scene->directional_light.cascade_ranges[2], 0.f, 500.f);
			ImGui::Text("Z-mults");
			ImGui::SliderFloat("##z1", &m_active_scene->directional_light.z_mults[0], 0.f, 10.f);
			ImGui::SliderFloat("##z2", &m_active_scene->directional_light.z_mults[1], 0.f, 10.f);
			ImGui::SliderFloat("##z3", &m_active_scene->directional_light.z_mults[2], 0.f, 10.f);

			ImGui::SliderFloat("Size", &m_active_scene->directional_light.light_size, 0.f, 150.f);
			ImGui::SliderFloat("Blocker search size", &m_active_scene->directional_light.blocker_search_size, 0.f, 50.f);

			ImGui::Checkbox("Shadows", &m_active_scene->directional_light.shadows_enabled);

			m_active_scene->directional_light.color = glm::vec3(light_color.x, light_color.y, light_color.z);
			m_active_scene->directional_light.SetLightDirection(light_dir);
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
		SceneEntity* p_dup_ent = nullptr;
		EditorEvent e;

		std::vector<SceneEntity*> ret;

		for (auto id : entities) {
			auto* p_original_ent = m_active_scene->GetEntity(id);
			if (m_simulate_mode_active)
				p_dup_ent = &m_active_scene->DuplicateEntityCallScript(*p_original_ent);
			else
				p_dup_ent = &p_original_ent->Duplicate();

			e.affected_entities.push_back(p_dup_ent->m_uuid());
			e.serialized_entities_before.push_back(SceneSerializer::SerializeEntityIntoString(*p_dup_ent));

			ret.push_back(p_dup_ent);
		}

		e.event_type = ENTITY_CREATE;
		m_event_stack.PushEvent(e);

		return ret;
	}

	void EditorLayer::DeleteEntitiesTracked(std::vector<uint64_t> entities) {
		EditorEvent e;
		e.event_type = ENTITY_DELETE;
		e.affected_entities = entities;
		for (auto id : entities) {
			DeselectEntity(id);

			auto* p_entity = m_active_scene->GetEntity(id);
			e.serialized_entities_before.push_back(SceneSerializer::SerializeEntityIntoString(*p_entity));
			m_active_scene->DeleteEntity(m_active_scene->GetEntity(id));
		}

		m_event_stack.PushEvent(e);
	}

	SceneEntity& EditorLayer::CreateEntityTracked(const std::string& name) {
		auto& ent = m_active_scene->CreateEntity(name);
		EditorEvent e;
		e.event_type = ENTITY_CREATE;
		e.affected_entities.push_back(ent.GetUUID());
		m_event_stack.PushEvent(e);
		return ent;
	}
}