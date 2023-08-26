#include "pch/pch.h"

#include <glfw/glfw3.h>
#include "../extern/Icons.h"
#include "../extern/imgui/backends/imgui_impl_glfw.h"
#include "../extern/imgui/backends/imgui_impl_opengl3.h"
#include "../extern/imgui/misc/cpp/imgui_stdlib.h"
#include "EditorLayer.h"
#include "scene/SceneSerializer.h"
#include "util/Timers.h"
#include "../extern/imguizmo/ImGuizmo.h"
#include "core/CodedAssets.h"
#include "core/AssetManager.h"
#include "yaml-cpp/yaml.h"
#include "scripting/ScriptingEngine.h"
#include "core/Input.h"

namespace ORNG {
	void EditorLayer::Init() {

		char buffer[ORNG_MAX_FILEPATH_SIZE];
		GetModuleFileName(nullptr, buffer, ORNG_MAX_FILEPATH_SIZE);
		m_executable_directory = buffer;
		m_executable_directory = m_executable_directory.substr(0, m_executable_directory.find_last_of('\\'));

		InitImGui();
		m_active_scene = std::make_unique<Scene>();
		mp_preview_scene = std::make_unique<Scene>();

		m_grid_mesh = std::make_unique<GridMesh>();
		m_grid_mesh->Init();
		mp_grid_shader = &Renderer::GetShaderLibrary().CreateShader("grid");
		mp_grid_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/GridVS.glsl");
		mp_grid_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/GridFS.glsl");
		mp_grid_shader->Init();

		mp_quad_shader = &Renderer::GetShaderLibrary().CreateShader("2d_quad");
		mp_quad_shader->AddStageFromString(GL_VERTEX_SHADER, CodedAssets::QuadVS);
		mp_quad_shader->AddStageFromString(GL_FRAGMENT_SHADER, CodedAssets::QuadFS);
		mp_quad_shader->Init();

		mp_picking_shader = &Renderer::GetShaderLibrary().CreateShader("picking");
		mp_picking_shader->AddStageFromString(GL_VERTEX_SHADER, CodedAssets::TransformVS);
		mp_picking_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/PickingFS.glsl");
		mp_picking_shader->Init();
		mp_picking_shader->AddUniform("comp_id");
		mp_picking_shader->AddUniform("transform");

		mp_highlight_shader = &Renderer::GetShaderLibrary().CreateShader("highlight");
		mp_highlight_shader->AddStageFromString(GL_VERTEX_SHADER, CodedAssets::TransformVS);
		mp_highlight_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/HighlightFS.glsl");
		mp_highlight_shader->Init();
		mp_highlight_shader->AddUniform("transform");

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

		mp_editor_pass_fb = &Renderer::GetFramebufferLibrary().CreateFramebuffer("editor_passes", true);
		mp_editor_pass_fb->AddShared2DTexture("shared_depth", Renderer::GetFramebufferLibrary().GetFramebuffer("gbuffer").GetTexture<Texture2D>("shared_depth"), GL_DEPTH_ATTACHMENT);
		mp_editor_pass_fb->AddShared2DTexture("Editor scene display", *mp_scene_display_texture, GL_COLOR_ATTACHMENT0);

		m_active_scene->LoadScene("");
		mp_editor_camera = std::make_unique<SceneEntity>(&*m_active_scene, m_active_scene->m_registry.create());
		mp_editor_camera->AddComponent<TransformComponent>()->SetPosition(0, 20, 0);
		mp_editor_camera->AddComponent<CameraComponent>()->MakeActive();
		mp_editor_camera->AddComponent<CharacterControllerComponent>();

		// Setup preview scene used for viewing materials on meshes
		mp_preview_scene->LoadScene("");
		mp_preview_scene->post_processing.bloom.intensity = 0.25;
		auto& cube_entity = mp_preview_scene->CreateEntity("Cube");
		cube_entity.AddComponent<MeshComponent>(&CodedAssets::GetCubeAsset());

		auto& cam_entity = mp_preview_scene->CreateEntity("Cam");
		auto* p_cam = cam_entity.AddComponent<CameraComponent>();
		p_cam->fov = 60.f;
		glm::vec3 cam_pos{ 3, 3, -3.0 };
		cam_entity.GetComponent<TransformComponent>()->SetPosition(cam_pos);
		mp_preview_scene->m_directional_light.SetLightDirection({ 0.1, 0.3, -1.0 });
		p_cam->target = glm::normalize(-cam_pos);
		p_cam->aspect_ratio = 1;
		p_cam->MakeActive();
		mp_preview_scene->Update(0);
		mp_preview_scene->skybox.LoadEnvironmentMap(m_executable_directory + "/res/textures/AdobeStock_247957406.jpeg");


		m_asset_preview_spec = m_color_render_texture_spec;
		m_asset_preview_spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
		m_asset_preview_spec.mag_filter = GL_LINEAR;
		m_asset_preview_spec.width = 256;
		m_asset_preview_spec.height = 256;
		m_asset_preview_spec.generate_mipmaps = true;

		CreateMaterialPreview(&CodedAssets::GetBaseMaterial());
		CreateMaterialPreview(&AssetManager::Get().m_replacement_material);
		CreateMeshPreview(&CodedAssets::GetCubeAsset());

		m_asset_listener.OnEvent = [this](const Events::ProjectEvent& t_event) {
			OnProjectEvent(t_event);
		};
		Events::EventManager::RegisterListener(m_asset_listener);

		std::string base_proj_dir = m_executable_directory + "\\projects\\base-project";
		if (!std::filesystem::exists(base_proj_dir)) {
			GenerateProject("base-project");
		}
		MakeProjectActive(base_proj_dir);
		// Make sure some project is active
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/scene/SceneEntity.h", "./res/scripts/includes/SceneEntity.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/core/Input.h", "./res/scripts/includes/Input.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/scene/Scene.h", "./res/scripts/includes/Scene.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/scripting/ScriptAPI.h", "./res/scripts/includes/ScriptAPI.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/components/Component.h", "./res/scripts/includes/Component.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/components/MeshComponent.h", "./res/scripts/includes/MeshComponent.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/components/ScriptComponent.h", "./res/scripts/includes/ScriptComponent.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/components/Lights.h", "./res/scripts/includes/Lights.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/extern/glm/glm/", "./res/scripts/includes/glm/", true);
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/components/TransformComponent.h", "./res/scripts/includes/TransformComponent.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/components/PhysicsComponent.h", "./res/scripts/includes/PhysicsComponent.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/components/CameraComponent.h", "./res/scripts/includes/CameraComponent.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/extern/entt/EnttSingleInclude.h", "./res/scripts/includes/EnttSingleInclude.h");
		auto& e = m_active_scene->CreateEntity("e");

		ORNG_CORE_INFO("Editor layer initialized");
	}




	void EditorLayer::OnProjectEvent(const Events::ProjectEvent& t_event) {
		switch (t_event.event_type) {
		case Events::ProjectEventType::MESH_LOADED: {
			auto* p_mesh = reinterpret_cast<MeshAsset*>(t_event.data_payload);
			CreateMeshPreview(p_mesh);

			std::string filepath{"./res/meshes/" + p_mesh->GetFilename().substr(p_mesh->GetFilename().find_last_of("/") + 1) + ".bin"};
			if (!std::filesystem::exists(filepath) && filepath.substr(0, filepath.size() - 4).find(".bin") == std::string::npos) {
				// Gen binary file if none exists
				SceneSerializer::SerializeMeshAssetBinary(filepath, *p_mesh);
			}

			break;
		}
		case Events::ProjectEventType::MATERIAL_LOADED:
		{
			auto* p_material = reinterpret_cast<Material*>(t_event.data_payload);
			m_materials_to_gen_previews.push_back(p_material);
			break;
		}
		case Events::ProjectEventType::MATERIAL_DELETED:
		{
			auto* p_material = reinterpret_cast<Material*>(t_event.data_payload);

			// Check if the material is in a loading queue, if it is it needs to be removed
			auto it = std::ranges::find(m_materials_to_gen_previews, p_material);
			if (it != m_materials_to_gen_previews.end())
				m_materials_to_gen_previews.erase(it);

			m_material_preview_textures.erase(p_material);
			break;
		}
		case Events::ProjectEventType::MESH_DELETED:
		{
			auto* p_mesh = reinterpret_cast<MeshAsset*>(t_event.data_payload);
			m_mesh_preview_textures.erase(p_mesh);
			break;
		}
		}
	}


	void EditorLayer::InitImGui() {
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.Fonts->AddFontDefault();
		io.FontDefault = io.Fonts->AddFontFromFileTTF(".\\res\\fonts\\Uniform.ttf", 18.0f);

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
		if (Window::IsKeyDown('K'))
			mp_editor_camera->GetComponent<CameraComponent>()->MakeActive();

		float ts = FrameTiming::GetTimeStep();

		if (Input::IsKeyDown('c')) {
			ORNG_CORE_ERROR("C DOWN EDITOR");
		}
		UpdateEditorCam();

		m_active_scene->Update(ts);
		// Updating here to work with the editor camera
		m_active_scene->terrain.UpdateTerrainQuadtree(mp_editor_camera->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0]);

		static float cooldown = 0;
		cooldown -= glm::min(cooldown, ts);
		if (cooldown > 10)
			return;

		// Keybind to focus on selected entities
		if (Window::IsKeyDown('F') && !m_selected_entity_ids.empty()) {
			glm::vec3 avg_pos = { 0, 0, 0 };
			int num_iters = 0;
			for (auto id : m_selected_entity_ids) {
				num_iters++;
				avg_pos += m_active_scene->GetEntity(id)->GetComponent<TransformComponent>()->m_pos;
			}

			// Smoothly move camera target to focus point
			glm::vec3 focused_target = glm::normalize(avg_pos / (float)num_iters - mp_editor_camera->GetComponent<TransformComponent>()->m_pos);
			mp_editor_camera->GetComponent<CameraComponent>()->target += focused_target * FrameTiming::GetTimeStep() * 0.01f;
			mp_editor_camera->GetComponent<CameraComponent>()->target = glm::normalize(mp_editor_camera->GetComponent<CameraComponent>()->target);
		}

		// Duplicate entity keybind
		if (Window::IsKeyDown(GLFW_KEY_LEFT_CONTROL) && Window::IsKeyDown('D')) {
			cooldown += 1000;

			std::vector<uint64_t> duplicate_ids;
			for (auto id : m_selected_entity_ids) {
				SceneEntity* p_entity = m_active_scene->GetEntity(id);
				if (!p_entity) continue;

				DuplicateEntity(p_entity);
				duplicate_ids.push_back(p_entity->GetUUID());
			}

			m_selected_entity_ids = duplicate_ids;
		}

	}


	void EditorLayer::UpdateEditorCam() {

		auto* p_cam = mp_editor_camera->GetComponent<CameraComponent>();
		auto* p_transform = mp_editor_camera->GetComponent<TransformComponent>();

		// Camera movement
		if (ImGui::IsMouseDown(1)) {
			glm::vec3 pos = mp_editor_camera->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0];
			glm::vec3 movement_vec{0.0, 0.0, 0.0};
			float time_elapsed = FrameTiming::GetTimeStep();
			movement_vec += p_cam->right * (float)Window::IsKeyDown(GLFW_KEY_D) * time_elapsed * p_cam->speed;
			movement_vec -= p_cam->right * (float)Window::IsKeyDown(GLFW_KEY_A) * time_elapsed * p_cam->speed;
			movement_vec += p_cam->target * (float)Window::IsKeyDown(GLFW_KEY_W) * time_elapsed * p_cam->speed;
			movement_vec -= p_cam->target * (float)Window::IsKeyDown(GLFW_KEY_S) * time_elapsed * p_cam->speed;
			movement_vec += p_cam->up * (float)Window::IsKeyDown(GLFW_KEY_E) * time_elapsed * p_cam->speed;
			movement_vec -= p_cam->up * (float)Window::IsKeyDown(GLFW_KEY_Q) * time_elapsed * p_cam->speed;
			p_transform->SetAbsolutePosition(pos + movement_vec);
		}

		// Camera rotation
		static glm::vec2 last_mouse_pos;
		if (ImGui::IsMouseClicked(1))
			last_mouse_pos = Window::GetMousePos();

		if (ImGui::IsMouseDown(1)) {
			float rotation_speed = 0.005f;
			glm::vec2 mouse_coords = Window::GetMousePos();
			glm::vec2 mouse_delta = -glm::vec2(mouse_coords.x - last_mouse_pos.x, mouse_coords.y - last_mouse_pos.y);

			p_cam->target = glm::rotate(mouse_delta.x * rotation_speed, p_cam->up) * glm::vec4(p_cam->target, 0);
			glm::fvec3 target_new = glm::rotate(mouse_delta.y * rotation_speed, glm::cross(p_cam->target, p_cam->up)) * glm::vec4(p_cam->target, 0);
			//constraint to stop lookAt flipping from y axis alignment
			if (target_new.y <= 0.9996f && target_new.y >= -0.996f) {
				p_cam->target = glm::normalize(target_new);
			}

			Window::SetCursorPos(last_mouse_pos.x, last_mouse_pos.y);
		}

		p_cam->UpdateFrustum();
	}



	void EditorLayer::RenderProfilingTimers() {
		static bool display_profiling_timers = ProfilingTimers::AreTimersEnabled();

		if (ImGui::Checkbox("Timers", &display_profiling_timers)) {
			ProfilingTimers::SetTimersEnabled(display_profiling_timers);
		}
		if (ProfilingTimers::AreTimersEnabled()) {
			for (auto& string : ProfilingTimers::GetTimerData()) {
				ImGui::Text(string.c_str());
			}
			ProfilingTimers::UpdateTimers(FrameTiming::GetTimeStep());
		}
	}




	void EditorLayer::RenderUI() {
		ORNG_PROFILE_FUNC();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();
		RenderErrorMessages();
		RenderToolbar();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 5);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 5));
		ImGui::SetNextWindowPos(ImVec2(0, toolbar_height));
		ImGui::SetNextWindowSize(ImVec2(400, Window::GetHeight() - toolbar_height));
		ImGui::Begin("##left window", (bool*)0, 0);
		ImGui::End();

		ImGui::SetNextWindowPos(ImVec2(Window::GetWidth() - 450, toolbar_height));
		ImGui::SetNextWindowSize(ImVec2(450, Window::GetHeight() - toolbar_height));
		if (ImGui::Begin("##right window", (bool*)0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
			DisplayEntityEditor();

		}
		ImGui::End();

		if (ImGui::Begin("Debug")) {
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::Text(std::format("Draw calls: {}", Renderer::Get().m_draw_call_amount).c_str());

			RenderProfilingTimers();

			Renderer::ResetDrawCallCounter();

		}
		ImGui::End();

		if (mp_selected_material)
			RenderMaterialEditorSection();

		if (mp_selected_texture)
			RenderTextureEditorSection();


		RenderSceneGraph();
		ShowAssetManager();

		ImGui::PopStyleVar(); // window border size
		ImGui::PopStyleVar(); // window padding


		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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
			ImGui::InputText("##e", &project_name);

			static std::string err_msg = "";
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
			ImGui::TextWrapped(err_msg.c_str());
			ImGui::PopStyleColor();

			if (ImGui::Button("Generate")) {
				if (GenerateProject(project_name)) {
					MakeProjectActive(m_executable_directory + "/projects/" + project_name);
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

	void EditorLayer::CreateMeshPreview(MeshAsset* p_asset) {

		auto p_tex = std::make_shared<Texture2D>(p_asset->GetFilename().substr(p_asset->GetFilename().find_last_of("/") + 1) + " - Mesh preview");
		p_tex->SetSpec(m_asset_preview_spec);
		m_mesh_preview_textures[p_asset] = p_tex;

		// Scale mesh so it always fits in camera frustum
		glm::vec3 extents = p_asset->GetAABB().max - p_asset->GetAABB().center;
		float largest_extent = glm::max(glm::max(extents.x, extents.y), extents.z);
		glm::vec3 scale_factor = glm::vec3(1.0, 1.0, 1.0) / largest_extent;

		mp_preview_scene->GetEntity("Cube")->GetComponent<TransformComponent>()->SetScale(scale_factor);
		mp_preview_scene->GetEntity("Cube")->GetComponent<MeshComponent>()->SetMeshAsset(p_asset);
		mp_preview_scene->Update(0);
		SceneRenderer::SceneRenderingSettings settings;
		SceneRenderer::SetActiveScene(&*mp_preview_scene);
		settings.p_output_tex = &*p_tex;

		SceneRenderer::SceneRenderingOutput output = SceneRenderer::RenderScene(settings);

		GL_StateManager::BindTexture(GL_TEXTURE_2D, p_tex->GetTextureHandle(), GL_TEXTURE0, true);
		glGenerateMipmap(GL_TEXTURE_2D);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, 0, GL_TEXTURE0, true);

	}

	void EditorLayer::CreateMaterialPreview(const Material* p_material) {
		auto p_tex = std::make_shared<Texture2D>(p_material->name + " - Material preview");
		p_tex->SetSpec(m_asset_preview_spec);
		m_material_preview_textures[p_material] = p_tex;


		auto* p_mesh = mp_preview_scene->GetEntity("Cube")->GetComponent<MeshComponent>();
		if (p_mesh->GetMeshData() != &CodedAssets::GetCubeAsset())
			p_mesh->SetMeshAsset(&CodedAssets::GetCubeAsset());
		mp_preview_scene->GetEntity("Cube")->GetComponent<TransformComponent>()->SetScale(1.0, 1.0, 1.0);

		mp_preview_scene->Update(0);
		for (int i = 0; i < p_mesh->m_materials.size(); i++) {
			p_mesh->SetMaterialID(i, p_material);
		}
		mp_preview_scene->Update(0);

		SceneRenderer::SceneRenderingSettings settings;
		SceneRenderer::SetActiveScene(&*mp_preview_scene);
		settings.p_output_tex = &*p_tex;
		SceneRenderer::SceneRenderingOutput output = SceneRenderer::RenderScene(settings);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, p_tex->GetTextureHandle(), GL_TEXTURE0, true);
		glGenerateMipmap(GL_TEXTURE_2D);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, 0, GL_TEXTURE0, true);

	}


	void EditorLayer::RenderToolbar() {
		ImGui::SetNextWindowSize(ImVec2(Window::GetWidth(), toolbar_height));
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		if (ImGui::Begin("##Toolbar", (bool*)0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
			if (ImGui::Button("Files")) {
				ImGui::OpenPopup("FilePopup");
			}
			static std::vector<std::string> file_options{"New project", "Open project", "Save project"};
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
					MakeProjectActive(filepath.substr(0, filepath.find_last_of('/')));
				};

				ShowFileExplorer(m_executable_directory + "/projects", valid_extensions, success_callback);
				selected_component = 0;
				break;
			}
			case 3:
				SceneSerializer::SerializeScene(*m_active_scene, "scene.yml");
			}


			ImGui::SameLine();
			std::string sep_text = "Project: " + m_current_project_directory.substr(m_current_project_directory.find_last_of("\\") + 1);
			ImGui::SeparatorText(sep_text.c_str());
		}
		ImGui::End();
	}



	void EditorLayer::RenderDisplayWindow() {
		Renderer::GetFramebufferLibrary().UnbindAllFramebuffers();
		GL_StateManager::DefaultClearBits();

		if (ImGui::IsMouseDoubleClicked(0) && !ImGui::GetIO().WantCaptureMouse) {
			DoPickingPass();
		}


		// Generate previews at this stage, delayed as if done during the ImGui rendering phase ImGui will stop rendering on that frame, causing a UI flicker
		for (auto* p_material : m_materials_to_gen_previews) {
			CreateMaterialPreview(p_material);
		}
		if (mp_selected_material) {
			CreateMaterialPreview(mp_selected_material);
		}
		m_materials_to_gen_previews.clear();

		SceneRenderer::SceneRenderingSettings settings;
		SceneRenderer::SetActiveScene(&*m_active_scene);
		settings.p_output_tex = &*mp_scene_display_texture;
		settings.p_cam_override = static_cast<CameraComponent*>(&*mp_editor_camera->GetComponent<CameraComponent>());
		SceneRenderer::SceneRenderingOutput output = SceneRenderer::RenderScene(settings);

		mp_editor_pass_fb->Bind();
		DoSelectedEntityHighlightPass();
		RenderGrid();

		Renderer::GetFramebufferLibrary().UnbindAllFramebuffers();
		mp_quad_shader->ActivateProgram();
		GL_StateManager::BindTexture(GL_TEXTURE_2D, mp_scene_display_texture->GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR, true);

		// Render scene texture to editor display window (currently just fullscreen quad)
		glDisable(GL_DEPTH_TEST);
		Renderer::DrawQuad();
		glEnable(GL_DEPTH_TEST);
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
		std::ofstream s{project_path + "/scene.yml"};
		s << ORNG_BASE_SCENE_YAML;
		s.close();

		std::filesystem::create_directory(project_path + "/res");
		std::filesystem::create_directory(project_path + "/res/meshes");
		std::filesystem::create_directory(project_path + "/res/textures");
		std::filesystem::create_directory(project_path + "/res/scripts");
		std::filesystem::create_directory(project_path + "/res/scripts/includes");
		std::filesystem::create_directory(project_path + "/res/scripts/bin");
		std::filesystem::create_directory(project_path + "/res/shaders");



		// Include header files for API so intellisense works
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/scene/SceneEntity.h", project_path + "/res/scripts/includes/SceneEntity.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/core/Input.h", project_path + "/res/scripts/includes/Input.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/extern/glm/glm/", project_path + "/res/scripts/includes/glm/", true);
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/scene/Scene.h", project_path + "/res/scripts/includes/Scene.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/scripting/ScriptAPI.h", project_path + "/res/scripts/includes/ScriptAPI.h");
		// Components
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/components/Component.h", project_path + "/res/scripts/includes/Component.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/components/MeshComponent.h", project_path + "/res/scripts/includes/MeshComponent.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/components/ScriptComponent.h", project_path + "/res/scripts/includes/ScriptComponent.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/components/Lights.h", project_path + "/res/scripts/includes/Lights.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/components/TransformComponent.h", project_path + "/res/scripts/includes/TransformComponent.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/components/PhysicsComponent.h", project_path + "/res/scripts/includes/PhysicsComponent.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/headers/components/CameraComponent.h", project_path + "/res/scripts/includes/CameraComponent.h");
		HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/extern/entt/EnttSingleInclude.h", project_path + "/res/scripts/includes/EnttSingleInclude.h");

		return true;

	}



	bool EditorLayer::MakeProjectActive(const std::string& folder_path) {
		if (std::filesystem::exists(folder_path) &&
			std::filesystem::exists(folder_path + "/scene.yml")) {
			std::ifstream stream(folder_path + "/scene.yml");
			std::stringstream str_stream;
			str_stream << stream.rdbuf();
			stream.close();

			YAML::Node data = YAML::Load(str_stream.str());
			if (!data.IsDefined() || data.IsNull() || !data["Scene"]) {
				ORNG_CORE_ERROR("Scene.yml file has invalid structure, likely corrupted");
				return false;
			}

			std::filesystem::current_path(folder_path);
			m_current_project_directory = folder_path;

			mp_selected_material = nullptr;
			mp_selected_texture = nullptr;
			m_selected_entity_ids.clear();
			glm::vec3 cam_pos = mp_editor_camera->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0];
			m_active_scene->m_physics_system.SetIsPaused(true);
			mp_editor_camera = nullptr; // Delete explicitly here to properly remove it from the scene before unloading

			m_active_scene->UnloadScene();
			AssetManager::ClearAll();
			//for (const auto& entry : std::filesystem::directory_iterator(".\\res\\scripts")) {
				//if (entry.path().extension().string() == ".cpp")
					//AssetManager::AddScriptAsset(entry.path().string());
			//}
			m_active_scene->LoadScene("scene.yml");


			mp_editor_camera = std::make_unique<SceneEntity>(&*m_active_scene, m_active_scene->m_registry.create());
			auto* p_transform = mp_editor_camera->AddComponent<TransformComponent>();
			p_transform->SetAbsolutePosition(cam_pos);
			mp_editor_camera->AddComponent<CameraComponent>();
			mp_editor_camera->GetComponent<CameraComponent>()->MakeActive();
		}
		else {
			ORNG_CORE_ERROR("Project folder/scene.yml path invalid");
			return false;
		}
		return true;
	}



	SceneEntity* EditorLayer::DuplicateEntity(SceneEntity* p_original) {
		SceneEntity& new_entity = m_active_scene->CreateEntity(p_original->name + " - Duplicate");
		std::string str = SceneSerializer::SerializeEntityIntoString(*p_original);
		SceneSerializer::DeserializeEntityFromString(*m_active_scene, str, new_entity);

		auto* p_relation_comp = p_original->GetComponent<RelationshipComponent>();
		SceneEntity* p_current_child = m_active_scene->GetEntity(p_relation_comp->first);
		while (p_current_child) {
			DuplicateEntity(p_current_child)->SetParent(new_entity);
			p_current_child = m_active_scene->GetEntity(p_current_child->GetComponent<RelationshipComponent>()->next);
		}
		return &new_entity;
	}



	void EditorLayer::RenderCreationWidget(SceneEntity* p_entity, bool trigger) {

		const char* names[6] = { "Pointlight", "Spotlight", "Mesh", "Camera", "Physics", "Script" };

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

		auto* entity = p_entity ? p_entity : &m_active_scene->CreateEntity("New entity");
		m_selected_entity_ids.clear(); // Clear selected ids as now only the new entity should be selected
		SelectEntity(entity->GetUUID());

		switch (selected_component) {
		case 0:
			entity->AddComponent<PointLightComponent>();
			break;
		case 1:
			entity->AddComponent<SpotLightComponent>();
			break;
		case 2:
			entity->AddComponent<MeshComponent>(&CodedAssets::GetCubeAsset());
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
		}
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
			glm::uvec2 id_vec{half_id_1, half_id_2};

			mp_picking_shader->SetUniform("comp_id", id_vec);
			mp_picking_shader->SetUniform("transform", mesh.GetEntity()->GetComponent<TransformComponent>()->GetMatrix());

			Renderer::DrawMeshInstanced(mesh.GetMeshData(), 1);
		}

		glm::vec2 mouse_coords = glm::min(glm::max(Window::GetMousePos(), glm::vec2(1, 1)), glm::vec2(Window::GetWidth() - 1, Window::GetHeight() - 1));

		uint32_t* pixels = new uint32_t[2];
		glReadPixels(mouse_coords.x, Window::GetHeight() - mouse_coords.y, 1, 1, GL_RG_INTEGER, GL_UNSIGNED_INT, pixels);
		uint64_t current_entity_id = ((uint64_t)pixels[0] << 32) | pixels[1];
		delete[] pixels;

		if (!Window::IsKeyDown(GLFW_KEY_LEFT_CONTROL))
			m_selected_entity_ids.clear();

		SelectEntity(current_entity_id);
	}




	void EditorLayer::DoSelectedEntityHighlightPass() {
		for (auto id : m_selected_entity_ids) {
			auto* current_entity = m_active_scene->GetEntity(id);

			if (!current_entity || !current_entity->HasComponent<MeshComponent>())
				continue;

			MeshComponent* meshc = current_entity->GetComponent<MeshComponent>();

			mp_editor_pass_fb->Bind();
			mp_highlight_shader->ActivateProgram();

			mp_highlight_shader->SetUniform("transform", meshc->GetEntity()->GetComponent<TransformComponent>()->GetMatrix());
			glDisable(GL_DEPTH_TEST);
			Renderer::DrawMeshInstanced(meshc->GetMeshData(), 1);
			glEnable(GL_DEPTH_TEST);
		}

	}




	void EditorLayer::RenderGrid() {
		GL_StateManager::BindSSBO(m_grid_mesh->m_ssbo_handle, GL_StateManager::SSBO_BindingPoints::TRANSFORMS);
		m_grid_mesh->CheckBoundary(mp_editor_camera->GetComponent<TransformComponent>()->GetPosition());
		mp_grid_shader->ActivateProgram();
		Renderer::DrawVAO_ArraysInstanced(GL_LINES, m_grid_mesh->m_vao, ceil(m_grid_mesh->grid_width / m_grid_mesh->grid_step) * 2);
	}


	void NameWithTooltip(const std::string& name) {
		ImGui::SeparatorText(name.c_str());
		// Tooltip to reveal full name in case it overflows
		if (ImGui::BeginItemTooltip()) {
			ImGui::Text(name.c_str());
			ImGui::EndTooltip();
		}
	}

	bool CenteredImageButton(ImTextureID id, ImVec2 size) {
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
		float padding = (ImGui::GetContentRegionAvail().x - size.x - ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().ItemSpacing.x) / 2.0;
		ImGui::Dummy(ImVec2(padding / 2.0, 0));
		ImGui::SameLine();

		bool ret = ImGui::ImageButton(id, size, ImVec2(0, 1), ImVec2(1, 0));
		ImGui::PopStyleVar();
		return ret;
	}


	void EditorLayer::ShowAssetManager() {

		int window_height = glm::clamp(static_cast<int>(Window::GetHeight()) / 4, 100, 500);
		int window_width = Window::GetWidth() - 850;
		ImVec2 image_button_size = { glm::clamp(window_width / 8.f, 75.f, 150.f) , 150 };
		int column_count = glm::max((int)(window_width / (image_button_size.x + 40)), 1);
		ImGui::SetNextWindowSize(ImVec2(window_width, window_height));
		ImGui::SetNextWindowPos(ImVec2(400, Window::GetHeight() - window_height));

		ImGui::Begin("Assets", (bool*)0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
		ImGui::BeginTabBar("Selection");


		if (ImGui::BeginTabItem("Meshes")) // MESH TAB
		{
			if (ImGui::Button("Add mesh")) // MESH FILE EXPLORER
			{

				wchar_t valid_extensions[MAX_PATH] = L"Mesh Files: *.obj;*.fbx\0*.obj;*.fbx\0";

				//setting up file explorer callbacks
				std::function<void(std::string)> success_callback = [this](std::string filepath) {
					MeshAsset* asset = AssetManager::CreateMeshAsset(filepath);
					AssetManager::LoadMeshAsset(asset);
				};


				ShowFileExplorer("", valid_extensions, success_callback);

			} // END MESH FILE EXPLORER

			static MeshAsset* p_dragged_mesh = nullptr;
			if (ImGui::BeginTable("Meshes", column_count)) // MESH VIEWING TABLE
			{
				for (auto* p_mesh_asset : AssetManager::Get().m_meshes)
				{
					ImGui::PushID(p_mesh_asset);
					ImGui::TableNextColumn();

					std::string name = p_mesh_asset->GetFilename().substr(p_mesh_asset->GetFilename().find_last_of('/') + 1);
					NameWithTooltip(name);
					if (p_mesh_asset->m_is_loaded)
					{
						CenteredImageButton(ImTextureID(m_mesh_preview_textures[p_mesh_asset]->GetTextureHandle()), image_button_size);
						if (ImGui::BeginDragDropSource()) {
							p_dragged_mesh = p_mesh_asset;
							ImGui::SetDragDropPayload("MESH", &p_dragged_mesh, sizeof(MeshAsset*));
							ImGui::EndDragDropSource();
						}

					}
					else
						ImGui::TextColored(ImVec4(1, 0, 0, 1), "Loading...");


					ImGui::PopID();
				}

				ImGui::EndTable();
				ImGui::EndTabItem();

			} // END MESH VIEWING TABLE
		} //	 END MESH TAB


		if (ImGui::BeginTabItem("Textures")) // TEXTURE TAB
		{
			if (ImGui::Button("Add texture")) {

				static std::string file_extension = "";
				wchar_t valid_extensions[MAX_PATH] = L"Texture Files: *.png;*.jpg;*.jpeg;*.hdr\0*.png;*.jpg;*.jpeg;*.hdr\0";

				std::function<void(std::string)> success_callback = [this](std::string filepath) {
					// Copy file so asset can be saved with project and accessed relatively
					std::string new_filepath = "./res/textures/" + filepath.substr(filepath.find_last_of("/") + 1);
					if (!std::filesystem::exists(new_filepath)) {
						HandledFileSystemCopy(filepath, new_filepath);
						m_current_2d_tex_spec.filepath = new_filepath;
						AssetManager::CreateTexture2D(m_current_2d_tex_spec);
					}
					else {
						ORNG_CORE_ERROR("Texture asset '{0}' not added, already found in project files", new_filepath);
					}
				};
				ShowFileExplorer("", valid_extensions, success_callback);

			}


			static Texture2D* p_dragged_texture = nullptr;
			// Create table for textures 
			if (ImGui::BeginTable("Textures", column_count)); // TEXTURE VIEWING TABLE
			{
				// Push textures into table 
				for (auto* p_texture : AssetManager::Get().m_2d_textures)
				{
					bool deletion_flag = false;
					ImGui::PushID(p_texture);
					ImGui::TableNextColumn();

					NameWithTooltip(p_texture->m_spec.filepath.substr(p_texture->m_spec.filepath.find_last_of('/') + 1).c_str());

					if (CenteredImageButton(ImTextureID(p_texture->GetTextureHandle()), image_button_size)) {
						mp_selected_texture = p_texture;
						m_current_2d_tex_spec = mp_selected_texture->m_spec;
					};

					if (ImGui::BeginDragDropSource()) {
						p_dragged_texture = p_texture;
						ImGui::SetDragDropPayload("TEXTURE", &p_dragged_texture, sizeof(Texture2D*));
						ImGui::EndDragDropSource();
					}

					// Deletion popup
					if (ImGui::IsItemHovered() && Window::IsMouseButtonDown(GLFW_MOUSE_BUTTON_2)) {
						ImGui::OpenPopup("my_select_popup");
					}

					if (ImGui::BeginPopup("my_select_popup"))
					{
						if (ImGui::Selectable("Delete")) { // Base material not deletable
							deletion_flag = true;
						}
						ImGui::EndPopup();
					}

					ImGui::PopID();

					if (deletion_flag) {
						AssetManager::DeleteTexture(p_texture);
					}
				}

				ImGui::EndTable();
			} // END TEXTURE VIEWING TABLE
			ImGui::EndTabItem();
		} // END TEXTURE TAB


		if (ImGui::BeginTabItem("Materials")) { // MATERIAL TAB
			if (ImGui::IsItemActive()) {
				for (auto* p_material : AssetManager::Get().m_materials) {
					m_materials_to_gen_previews.push_back(p_material);
				}
			}

			if (ImGui::Button("Create material")) {
				AssetManager::CreateMaterial();
			}
			if (ImGui::BeginTable("Material viewer", column_count)) { //MATERIAL VIEWING TABLE

				for (auto* p_material : AssetManager::Get().m_materials) {

					if (!m_material_preview_textures.contains(p_material))
						// No material preview so proceeding will lead to a crash
						continue;

					ImGui::TableNextColumn();
					ImGui::PushID(p_material);

					bool deletion_flag = false;
					unsigned int tex_id = p_material->base_color_texture ? p_material->base_color_texture->m_texture_obj : CodedAssets::GetBaseTexture().GetTextureHandle();

					NameWithTooltip(p_material->name.c_str());

					if (CenteredImageButton(ImTextureID(m_material_preview_textures[p_material]->GetTextureHandle()), image_button_size))
						mp_selected_material = p_material;


					static Material* p_dragged_material = nullptr;
					if (ImGui::BeginDragDropSource()) {
						p_dragged_material = p_material;
						ImGui::SetDragDropPayload("MATERIAL", &p_dragged_material, sizeof(Material*));
						ImGui::EndDragDropSource();
					}

					// Deletion popup
					if (ImGui::IsItemHovered() && Window::IsMouseButtonDown(GLFW_MOUSE_BUTTON_2)) {
						ImGui::OpenPopup("my_select_popup");
					}

					if (ImGui::BeginPopup("my_select_popup"))
					{
						if (p_material->uuid() != ORNG_REPLACEMENT_MATERIAL_ID && ImGui::Selectable("Delete")) { // Base material not deletable
							deletion_flag = true;
						}
						if (ImGui::Selectable("Duplicate")) {
							auto* p_new_material = AssetManager::CreateMaterial();
							*p_new_material = *p_material;
							// Render a preview for material
							m_materials_to_gen_previews.push_back(p_new_material);
						}
						ImGui::EndPopup();
					}


					if (deletion_flag) {
						AssetManager::DeleteMaterial(p_material->uuid());
						mp_selected_material = p_material == mp_selected_material ? nullptr : mp_selected_material;
					}

					ImGui::PopID();
				}


				ImGui::EndTable();
			} //END MATERIAL VIEWING TABLE
			ImGui::EndTabItem();
		} //END MATERIAL TAB


		if (ImGui::BeginTabItem("Scripts")) {
			if (ImGui::Button("Create script")) {
				wchar_t valid_extensions[MAX_PATH] = L"Mesh Files: *.obj;*.fbx\0*.obj;*.fbx\0";
				std::string script_path = m_current_project_directory + "/res/scripts/" + std::to_string(UUID()()) + ".cpp";
				HandledFileSystemCopy(ORNG_CORE_MAIN_DIR "/src/scripting/ScriptingTemplate.cpp", script_path);
				std::string open_file_command = "start " + script_path;
				std::system(open_file_command.c_str());
			}

			if (ImGui::BeginTable("##script table", column_count)) {
				for (const auto& entry : std::filesystem::directory_iterator(m_current_project_directory + "\\res\\scripts")) {
					if (std::filesystem::is_regular_file(entry) && entry.path().extension().string() == ".cpp") {
						ImGui::TableNextColumn();

						std::string entry_path = entry.path().string();
						ImGui::PushID(entry_path.c_str());
						NameWithTooltip(entry_path.substr(entry_path.find_last_of("\\") + 1));

						bool is_loaded = AssetManager::Get().m_scripts.contains(entry_path);
						if (is_loaded)
							ImGui::TextColored(ImVec4(0, 1, 0, 1), "Loaded");
						else
							ImGui::TextColored(ImVec4(1, 0, 0, 1), "Not loaded");

						ImGui::Button(ICON_FA_FILE, image_button_size);
						static std::string dragged_script_filepath;
						if (ImGui::BeginDragDropSource()) {
							dragged_script_filepath = entry_path;
							ImGui::SetDragDropPayload("SCRIPT", &dragged_script_filepath, sizeof(std::string));
							ImGui::EndDragDropSource();
						}

						// Deletion popup
						if (ImGui::IsItemHovered() && Window::IsMouseButtonDown(GLFW_MOUSE_BUTTON_2)) {
							ImGui::OpenPopup("script_option_popup");
						}

						if (ImGui::BeginPopup("script_option_popup"))
						{
							if ((!is_loaded && ImGui::Selectable("Load"))) {
								if (!AssetManager::AddScriptAsset(entry_path))
									GenerateErrorMessage("AssetManager::AddScriptAsset failed");
							}
							else if (is_loaded && ImGui::Selectable("Reload")) {
								bool successful_reload = false;

								// Reload script and reconnect it to script components previously using it
								if (AssetManager::DeleteScriptAsset(entry_path)) {
									if (auto* p_symbols = AssetManager::AddScriptAsset(entry_path)) {
										for (auto [entity, script_comp] : m_active_scene->m_registry.view<ScriptComponent>().each()) {
											if (script_comp.script_filepath == entry_path) {
												script_comp.OnCreate = p_symbols->OnCreate;
												script_comp.OnUpdate = p_symbols->OnUpdate;
												script_comp.OnDestroy = p_symbols->OnDestroy;
											}
										}
										successful_reload = true;
									}
								}

								if (!successful_reload)
									GenerateErrorMessage("Failed to reload script");

							}
							ImGui::EndPopup();
						}

						ImGui::PopID(); // entry_path.c_str()
					}
				}

				ImGui::EndTable();
			}

			ImGui::EndTabItem();
		}


		ImGui::EndTabBar();
		ImGui::End();
	}




	void EditorLayer::RenderTextureEditorSection() {

		if (ImGui::Begin("Texture editor")) {

			if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
				// Hide this window
				mp_selected_texture = nullptr;
				goto window_end;
			}

			if (ImGui::BeginTable("##t", 2)) {
				ImGui::TableNextColumn();
				ImGui::TextWrapped(mp_selected_texture->GetSpec().filepath.c_str());
				ImGui::Separator();

				const char* wrap_modes[] = { "REPEAT", "CLAMP TO EDGE" };
				const char* filter_modes[] = { "LINEAR", "NEAREST" };
				static int selected_wrap_mode = m_current_2d_tex_spec.wrap_params == GL_REPEAT ? 0 : 1;
				static int selected_filter_mode = m_current_2d_tex_spec.mag_filter == GL_LINEAR ? 0 : 1;

				ImGui::Checkbox("SRGB", reinterpret_cast<bool*>(&m_current_2d_tex_spec.srgb_space));

				ImGui::Text("Wrap mode");
				ImGui::SameLine();
				ImGui::Combo("##Wrap mode", &selected_wrap_mode, wrap_modes, IM_ARRAYSIZE(wrap_modes));
				m_current_2d_tex_spec.wrap_params = selected_wrap_mode == 0 ? GL_REPEAT : GL_CLAMP_TO_EDGE;

				ImGui::Text("Filtering");
				ImGui::SameLine();
				ImGui::Combo("##Filter mode", &selected_filter_mode, filter_modes, IM_ARRAYSIZE(filter_modes));
				m_current_2d_tex_spec.mag_filter = selected_filter_mode == 0 ? GL_LINEAR : GL_NEAREST;
				m_current_2d_tex_spec.min_filter = selected_filter_mode == 0 ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST;

				if (ImGui::Button("Load")) {
					mp_selected_texture->SetSpec(m_current_2d_tex_spec);
					mp_selected_texture->LoadFromFile();

				}
				m_current_2d_tex_spec.generate_mipmaps = true;

				ImGui::TableNextColumn();
				float size = glm::min(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
				CenteredImageButton(ImTextureID(mp_selected_texture->GetTextureHandle()), ImVec2(size, size));
				ImGui::Spacing();
				ImGui::EndTable();
			}

		}

	window_end:
		ImGui::End();
	}




	void EditorLayer::RenderSkyboxEditor() {
		if (H1TreeNode("Skybox")) {

			wchar_t valid_extensions[MAX_PATH] = L"Texture Files: *.png;*.jpg;*.jpeg;*.hdr\0*.png;*.jpg;*.jpeg;*.hdr\0";

			std::function<void(std::string)> file_explorer_callback = [this](std::string filepath) {
				// Check if texture is an asset or not, if not, add it
				std::string new_filepath = "./res/textures/" + filepath.substr(filepath.find_last_of("/") + 1);
				if (!std::filesystem::exists(new_filepath)) {
					HandledFileSystemCopy(filepath, new_filepath);
				}
				m_active_scene->skybox.LoadEnvironmentMap(new_filepath);
			};

			if (ImGui::Button("Load skybox texture")) {
				ShowFileExplorer("", valid_extensions, file_explorer_callback);
			}
			ImGui::SameLine();
			ImGui::SmallButton("?");
			if (ImGui::BeginItemTooltip()) {
				ImGui::Text("Converts an equirectangular image into a cubemap for use in a skybox. For best results, use HDRI's!");
				ImGui::EndTooltip();
			}

		}
	}




	void EditorLayer::RenderMaterialEditorSection() {

		if (ImGui::Begin("Material editor")) {

			if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
				// Hide this tree node
				mp_selected_material = nullptr;
				goto window_end;
			}


			ImGui::Text("Name: ");
			ImGui::SameLine();
			ImGui::InputText("##name input", &mp_selected_material->name);
			ImGui::Spacing();

			RenderMaterialTexture("Base", mp_selected_material->base_color_texture);
			RenderMaterialTexture("Normal", mp_selected_material->normal_map_texture);
			RenderMaterialTexture("Roughness", mp_selected_material->roughness_texture);
			RenderMaterialTexture("Metallic", mp_selected_material->metallic_texture);
			RenderMaterialTexture("AO", mp_selected_material->ao_texture);
			RenderMaterialTexture("Displacement", mp_selected_material->displacement_texture);
			RenderMaterialTexture("Emissive", mp_selected_material->emissive_texture);

			ImGui::Text("Colors");
			ImGui::Spacing();
			ShowVec3Editor("Base color", mp_selected_material->base_color);

			if (!mp_selected_material->roughness_texture)
				ImGui::SliderFloat("Roughness", &mp_selected_material->roughness, 0.f, 1.f);

			if (!mp_selected_material->metallic_texture)
				ImGui::SliderFloat("Metallic", &mp_selected_material->metallic, 0.f, 1.f);

			if (!mp_selected_material->ao_texture)
				ImGui::SliderFloat("AO", &mp_selected_material->ao, 0.f, 1.f);

			ImGui::Checkbox("Emissive", &mp_selected_material->emissive);

			if (mp_selected_material->emissive || mp_selected_material->emissive_texture)
				ImGui::SliderFloat("Emissive strength", &mp_selected_material->emissive_strength, -10.f, 10.f);

			int num_parallax_layers = mp_selected_material->parallax_layers;
			if (mp_selected_material->displacement_texture) {
				ImGui::InputInt("Parallax layers", &num_parallax_layers);

				if (num_parallax_layers >= 0)
					mp_selected_material->parallax_layers = num_parallax_layers;

				ImGui::InputFloat("Parallax scale", &mp_selected_material->parallax_height_scale);
			}

			ShowVec2Editor("Tile scale", mp_selected_material->tile_scale);
		}

	window_end:

		ImGui::End();
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
			formatted_name += p_entity->HasComponent<PhysicsComponent>() ? " " ICON_FA_WIND : "";
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
		flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick;
		flags |= p_entity_relationship_comp->num_children > 0 ? ImGuiTreeNodeFlags_OpenOnArrow : 0;
		bool is_tree_node_open = ImGui::TreeNodeEx(formatted_name.c_str(), flags);
		if (ImGui::IsItemToggledOpen()) {
			auto it = std::ranges::find(open_tree_nodes, p_entity->GetUUID());
			if (it == open_tree_nodes.end())
				open_tree_nodes.push_back(p_entity->GetUUID());
			else
				open_tree_nodes.erase(it);
		}

		m_selected_entities_are_dragged |= ImGui::IsItemActive() && !ImGui::IsItemHovered();


		ImGui::PopStyleVar();
		ImGui::PopStyleColor();

		std::string popup_id = std::format("{}", p_entity->GetUUID()); // unique popup id


		if (ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax())) {
			// Drag entities into another entity node to make them children of it
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && m_selected_entities_are_dragged) {
				DeselectEntity(p_entity->GetUUID());
				for (auto id : m_selected_entity_ids) {
					m_active_scene->GetEntity(id)->SetParent(*p_entity);
				}

				m_selected_entities_are_dragged = false;
			}

			// Right click to open popup 
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
				ImGui::OpenPopup(popup_id.c_str());

		}
		// If clicked, select current entity
		if (ImGui::IsItemHovered() && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))) { // Doing this instead of IsActive() because IsActive wont trigger if ctrl is held down
			if (!Window::IsKeyDown(GLFW_KEY_LEFT_CONTROL)) // Only selecting one entity at a time
				m_selected_entity_ids.clear();


			if (Window::IsKeyDown(GLFW_KEY_LEFT_CONTROL) && VectorContains(m_selected_entity_ids, p_entity->GetUUID())) // Deselect entity from group of entities currently selected
				m_selected_entity_ids.erase(std::ranges::find(m_selected_entity_ids, p_entity->GetUUID()));
			else
				SelectEntity(p_entity->GetUUID());

		}

		// Popup opened above if node is right clicked
		if (ImGui::BeginPopup(popup_id.c_str()))
		{

			ImGui::SeparatorText("Options");
			if (ImGui::Selectable("Delete")) {
				ret = EntityNodeEvent::E_DELETE;
				ImGui::EndPopup();
				goto exit;
			}


			// Creates clone of entity, puts it in scene
			if (ImGui::Selectable("Duplicate")) {
				ret = EntityNodeEvent::E_DUPLICATE;
				ImGui::EndPopup();
				goto exit;
			}

			ImGui::EndPopup();
		}

	exit:
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
		if (ImGui::Begin("Scene graph")) {
			// Click anywhere on window to deselect entity nodes
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !Window::IsKeyDown(GLFW_KEY_LEFT_CONTROL))
				m_selected_entity_ids.clear();


			// Right click to bring up "new entity" popup
			RenderCreationWidget(nullptr, ImGui::IsWindowHovered() && Window::IsMouseButtonDown(GLFW_MOUSE_BUTTON_2));

			bool physics_paused = m_active_scene->m_physics_system.GetIsPaused();
			if (ImGui::RadioButton("Physics", !physics_paused))
				m_active_scene->m_physics_system.SetIsPaused(!physics_paused);

			ImGui::Text("Editor cam exposure");
			ImGui::SliderFloat("##exposure", &mp_editor_camera->GetComponent<CameraComponent>()->exposure, 0.f, 10.f);


			if (H2TreeNode("Entities")) {

				m_display_skybox_editor = EmptyTreeNode("Skybox");
				m_display_directional_light_editor = EmptyTreeNode("Directional Light");
				m_display_global_fog_editor = EmptyTreeNode("Global fog");
				m_display_terrain_editor = EmptyTreeNode("Terrain");
				m_display_bloom_editor = EmptyTreeNode("Bloom");

				// Render entity nodes, setup event capture
				EntityNodeEvent active_event = EntityNodeEvent::E_NONE;
				for (auto* p_entity : m_active_scene->m_entities) {
					if (p_entity->GetComponent<RelationshipComponent>()->parent != entt::null)
						continue;

					if (EntityNodeEvent e_event = RenderEntityNode(p_entity, 0); e_event == EntityNodeEvent::E_DUPLICATE || e_event == EntityNodeEvent::E_DELETE) {
						active_event = e_event;
					}
				}

				// Process node events
				if (active_event & EntityNodeEvent::E_DUPLICATE) {
					for (auto id : m_selected_entity_ids) {
						DuplicateEntity(m_active_scene->GetEntity(id));
					}
				}
				else if (active_event & EntityNodeEvent::E_DELETE) {
					for (auto id : m_selected_entity_ids) {
						m_active_scene->DeleteEntity(m_active_scene->GetEntity(id));
					}
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
		if (ImGui::Begin("Entity editor")) {

			if (m_display_directional_light_editor)
				RenderDirectionalLightEditor();
			if (m_display_global_fog_editor)
				RenderGlobalFogEditor();
			if (m_display_skybox_editor)
				RenderSkyboxEditor();
			if (m_display_terrain_editor)
				RenderTerrainEditor();
			if (m_display_bloom_editor)
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
			auto p_physics_comp = entity->GetComponent<PhysicsComponent>();
			auto* p_script_comp = entity->GetComponent<ScriptComponent>();

			std::vector<TransformComponent*> transforms;
			for (auto id : m_selected_entity_ids) {
				transforms.push_back(m_active_scene->GetEntity(id)->GetComponent<TransformComponent>());
			}
			std::string ent_text = std::format("Entity '{}'", entity->name);
			ImGui::InputText("Name", &entity->name);
			ImGui::Text(ent_text.c_str());


			//TRANSFORM
			if (H2TreeNode("Entity transform")) {
				RenderTransformComponentEditor(transforms);
			}


			//MESH
			if (meshc) {
				if (H2TreeNode("Mesh component")) {
					if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(1)) {
						entity->DeleteComponent<MeshComponent>();
					}
					if (meshc->mp_mesh_asset) {
						RenderMeshComponentEditor(meshc);
					}
				}

			}


			ImGui::PushID(plight);
			if (plight && H2TreeNode("Pointlight component")) {
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(1)) {
					entity->DeleteComponent<PointLightComponent>();
				}
				else {
					RenderPointlightEditor(plight);
				}
			}
			ImGui::PopID();

			ImGui::PushID(slight);
			if (slight && H2TreeNode("Spotlight component")) {
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(1)) {
					entity->DeleteComponent<SpotLightComponent>();
				}
				else {
					RenderSpotlightEditor(slight);
				}
			}
			ImGui::PopID();

			ImGui::PushID(p_cam);
			if (p_cam && H2TreeNode("Camera component")) {
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(1)) {
					entity->DeleteComponent<CameraComponent>();
				}
				RenderCameraEditor(p_cam);
			}
			ImGui::PopID();

			ImGui::PushID(p_physics_comp);
			if (p_physics_comp && H2TreeNode("Physics component")) {
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(1)) {
					entity->DeleteComponent<PhysicsComponent>();
				}
				else {
					RenderPhysicsComponentEditor(p_physics_comp);
				}
			}
			ImGui::PopID();

			if (p_script_comp && H2TreeNode("Script component")) {
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(1)) {
					entity->DeleteComponent<ScriptComponent>();
				}
				else {
					RenderScriptComponentEditor(p_script_comp);
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




	// EDITORS ------------------------------------------------------------------------

	void EditorLayer::RenderPhysicsMaterial(physx::PxMaterial* p_material) {
		float restitution = p_material->getRestitution();
		if (ClampedFloatInput("Restitution", &restitution, 0.f, 1.f)) {
			p_material->setRestitution(restitution);
		}

		float dynamic_friction = p_material->getDynamicFriction();
		if (ClampedFloatInput("Dynamic friction", &dynamic_friction, 0.f, 1.f)) {
			p_material->setDynamicFriction(dynamic_friction);
		}

		float static_friction = p_material->getStaticFriction();
		if (ClampedFloatInput("Static friction", &static_friction, 0.f, 1.f)) {
			p_material->setStaticFriction(static_friction);
		}
	}



	void EditorLayer::RenderScriptComponentEditor(ScriptComponent* p_script) {
		ImGui::PushID(p_script);

		NameWithTooltip(p_script->script_filepath.substr(p_script->script_filepath.find_last_of("\\") + 1).c_str());
		ImGui::Button(ICON_FA_FILE, ImVec2(100, 100));
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("SCRIPT")) {
				if (p_payload->DataSize == sizeof(std::string)) {
					ScriptSymbols* p_symbols = AssetManager::GetScriptAsset(*static_cast<std::string*>(p_payload->Data));
					p_script->OnCreate = p_symbols->OnCreate;
					p_script->OnUpdate = p_symbols->OnUpdate;
					p_script->OnDestroy = p_symbols->OnDestroy;
					p_script->script_filepath = *static_cast<std::string*>(p_payload->Data);
				}

			}
			ImGui::EndDragDropTarget();
		}

		ImGui::PopID(); // p_script
	}



	void EditorLayer::RenderPhysicsComponentEditor(PhysicsComponent* p_comp) {
		ImGui::SeparatorText("Collider geometry");

		if (ImGui::RadioButton("Box", p_comp->geometry_type == PhysicsComponent::BOX)) {
			p_comp->UpdateGeometry(PhysicsComponent::BOX);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Sphere", p_comp->geometry_type == PhysicsComponent::SPHERE)) {
			p_comp->UpdateGeometry(PhysicsComponent::SPHERE);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Mesh", p_comp->geometry_type == PhysicsComponent::TRIANGLE_MESH)) {
			p_comp->UpdateGeometry(PhysicsComponent::TRIANGLE_MESH);
		}


		ImGui::SeparatorText("Collider behaviour"); // Currently don't support dynamic triangle meshes
		if (ImGui::RadioButton("Dynamic", p_comp->rigid_body_type == PhysicsComponent::DYNAMIC)) {
			p_comp->SetBodyType(PhysicsComponent::DYNAMIC);
		}
		if (ImGui::RadioButton("Static", p_comp->rigid_body_type == PhysicsComponent::STATIC)) {
			p_comp->SetBodyType(PhysicsComponent::STATIC);
		}


		RenderPhysicsMaterial(p_comp->p_material);

	}




	void EditorLayer::RenderTransformComponentEditor(std::vector<TransformComponent*>& transforms) {


		static bool render_gizmos = true;

		ImGui::Checkbox("Gizmos", &render_gizmos);

		static bool absolute_mode = false;

		if (ImGui::Checkbox("Absolute", &absolute_mode)) {
			transforms[0]->SetAbsoluteMode(absolute_mode);
		}

		glm::vec3 matrix_translation = transforms[0]->m_pos;
		glm::vec3 matrix_rotation = transforms[0]->m_orientation;
		glm::vec3 matrix_scale = transforms[0]->m_scale;

		// UI section
		if (ShowVec3Editor("Tr", matrix_translation))
			std::ranges::for_each(transforms, [matrix_translation](TransformComponent* p_transform) {p_transform->SetPosition(matrix_translation); });

		if (ShowVec3Editor("Rt", matrix_rotation))
			std::ranges::for_each(transforms, [matrix_rotation](TransformComponent* p_transform) {p_transform->SetOrientation(matrix_rotation); });

		if (ShowVec3Editor("Sc", matrix_scale))
			std::ranges::for_each(transforms, [matrix_scale](TransformComponent* p_transform) {p_transform->SetScale(matrix_scale); });


		if (!render_gizmos)
			return;

		// Gizmos 
		ImGuiIO& io = ImGui::GetIO();
		ImGuizmo::BeginFrame();
		ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

		static ImGuizmo::OPERATION current_operation = ImGuizmo::TRANSLATE;
		static ImGuizmo::MODE current_mode = ImGuizmo::WORLD;


		if (Window::IsKeyDown(GLFW_KEY_1))
			current_operation = ImGuizmo::TRANSLATE;
		else if (Window::IsKeyDown(GLFW_KEY_2))
			current_operation = ImGuizmo::SCALE;
		else if (Window::IsKeyDown(GLFW_KEY_3))
			current_operation = ImGuizmo::ROTATE;

		ImGui::Text("Gizmo rendering");
		if (ImGui::RadioButton("World", current_mode == ImGuizmo::WORLD))
			current_mode = ImGuizmo::WORLD;
		ImGui::SameLine();
		if (ImGui::RadioButton("Local", current_mode == ImGuizmo::LOCAL))
			current_mode = ImGuizmo::LOCAL;

		glm::mat4 current_operation_matrix = transforms[0]->GetMatrix();

		static glm::mat4 delta_matrix;
		CameraComponent* p_cam = m_active_scene->m_camera_system.GetActiveCamera();

		if (ImGuizmo::Manipulate(&p_cam->GetViewMatrix()[0][0], &p_cam->GetProjectionMatrix()[0][0], current_operation, current_mode, &current_operation_matrix[0][0], &delta_matrix[0][0], nullptr) && ImGuizmo::IsUsing()) {

			ImGuizmo::DecomposeMatrixToComponents(&delta_matrix[0][0], &matrix_translation[0], &matrix_rotation[0], &matrix_scale[0]);

			auto base_abs_transforms = transforms[0]->GetAbsoluteTransforms();
			glm::vec3 base_abs_translation = base_abs_transforms[0];
			glm::vec3 base_abs_scale = base_abs_transforms[1];
			glm::vec3 base_abs_rotation = base_abs_transforms[2];

			glm::vec3 delta_translation = matrix_translation;
			glm::vec3 delta_scale = matrix_scale;
			glm::vec3 delta_rotation = matrix_rotation;

			for (auto* p_transform : transforms) {

				auto current_transforms = p_transform->GetAbsoluteTransforms();
				switch (current_operation) {
				case ImGuizmo::TRANSLATE:
					p_transform->SetPosition(p_transform->GetPosition() + delta_translation);
					break;
				case ImGuizmo::SCALE:
					p_transform->SetScale(p_transform->GetScale() * delta_scale);
					break;
				case ImGuizmo::ROTATE: // This will rotate multiple objects as one, using entity transform at m_selected_entity_ids[0] as origin
					glm::vec3 abs_rotation = current_transforms[2];
					glm::vec3 inherited_rotation = abs_rotation - p_transform->GetOrientation();
					glm::mat4 new_rotation_mat = ExtraMath::Init3DRotateTransform(delta_rotation.x, delta_rotation.y, delta_rotation.z) * ExtraMath::Init3DRotateTransform(abs_rotation.x, abs_rotation.y, abs_rotation.z);
					ImGuizmo::DecomposeMatrixToComponents(&new_rotation_mat[0][0], &matrix_translation[0], &matrix_rotation[0], &matrix_scale[0]);
					p_transform->SetOrientation(matrix_rotation);

					glm::vec3 abs_translation = current_transforms[0];
					glm::vec3 transformed_pos = abs_translation - base_abs_translation;
					glm::vec3 rotation_offset = glm::mat3(ExtraMath::Init3DRotateTransform(delta_rotation.x, delta_rotation.y, delta_rotation.z)) * transformed_pos; // rotate around transformed origin
					p_transform->SetAbsolutePosition(base_abs_translation + rotation_offset);
					break;
				}

			}
		};

	}




	Material* EditorLayer::RenderMaterialComponent(const Material* p_material) {
		Material* ret = nullptr;

		if (ImGui::ImageButton(ImTextureID(m_material_preview_textures[p_material]->GetTextureHandle()), ImVec2(100, 100), ImVec2(0, 1), ImVec2(1, 0))) {
			mp_selected_material = AssetManager::GetMaterial(p_material->uuid());
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




	void EditorLayer::RenderMaterialTexture(const char* name, Texture2D*& p_tex) {
		ImGui::PushID(p_tex);
		if (p_tex) {
			ImGui::Text(std::format("{} texture - {}", name, p_tex->m_name).c_str());
			if (ImGui::ImageButton(ImTextureID(p_tex->GetTextureHandle()), ImVec2(75, 75), ImVec2(0, 1), ImVec2(1, 0))) {
				mp_selected_texture = p_tex;
				m_current_2d_tex_spec = p_tex->m_spec;
			};
		}
		else {
			ImGui::Text(std::format("{} texture - NONE", name).c_str());
			ImGui::ImageButton(ImTextureID(0), ImVec2(75, 75));
		}

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("TEXTURE")) {
				if (p_payload->DataSize == sizeof(Texture2D*))
					p_tex = *static_cast<Texture2D**>(p_payload->Data);
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::IsItemHovered()) {
			if (Window::IsMouseButtonDown(GLFW_MOUSE_BUTTON_2)) // Delete texture from material
				p_tex = nullptr;
		}

		ImGui::PopID();
	}




	void EditorLayer::RenderMeshComponentEditor(MeshComponent* comp) {

		ImGui::PushID(comp);

		ImGui::SeparatorText("Mesh asset");
		ImGui::ImageButton(ImTextureID(m_mesh_preview_textures[comp->mp_mesh_asset]->GetTextureHandle()), ImVec2(100, 100), ImVec2(0, 1), ImVec2(1, 0));
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("MESH")) {
				if (p_payload->DataSize == sizeof(MeshAsset*))
					comp->SetMeshAsset(*static_cast<MeshAsset**>(p_payload->Data));
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::SeparatorText("Materials");
		for (int i = 0; i < comp->m_materials.size(); i++) {
			auto p_material = comp->m_materials[i];
			ImGui::PushID(i);

			if (auto* p_new_material = RenderMaterialComponent(p_material)) {
				comp->SetMaterialID(i, p_new_material);
			}

			ImGui::PopID();
		}


		ImGui::PopID();
	};




	void EditorLayer::RenderSpotlightEditor(SpotLightComponent* light) {

		float aperture = glm::degrees(acosf(light->m_aperture));
		glm::vec3 dir = light->m_light_direction_vec;

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

		ShowVec3Editor("Direction", dir);
		ShowColorVec3Editor("Color", light->color);

		light->SetLightDirection(dir.x, dir.y, dir.z);
		light->SetAperture(aperture);
	}




	void EditorLayer::RenderGlobalFogEditor() {
		if (H2TreeNode("Global fog")) {
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
			ShowColorVec3Editor("Color", m_active_scene->post_processing.global_fog.color);
		}
	}




	void EditorLayer::RenderBloomEditor() {
		if (H2TreeNode("Bloom")) {
			ImGui::SliderFloat("Intensity", &m_active_scene->post_processing.bloom.intensity, 0.f, 10.f);
			ImGui::SliderFloat("Threshold", &m_active_scene->post_processing.bloom.threshold, 0.0f, 50.0f);
			ImGui::SliderFloat("Knee", &m_active_scene->post_processing.bloom.knee, 0.f, 1.f);
		}
	}




	void EditorLayer::RenderTerrainEditor() {
		if (H2TreeNode("Terrain")) {
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
		if (H1TreeNode("Directional light")) {
			ImGui::Text("DIR LIGHT CONTROLS");

			static glm::vec3 light_dir = glm::vec3(0, 0.5, 0.5);
			static glm::vec3 light_color = m_active_scene->m_directional_light.color;

			ImGui::SliderFloat("X", &light_dir.x, -1.f, 1.f);
			ImGui::SliderFloat("Y", &light_dir.y, -1.f, 1.f);
			ImGui::SliderFloat("Z", &light_dir.z, -1.f, 1.f);
			ShowColorVec3Editor("Color", light_color);

			ImGui::Text("Cascade ranges");
			ImGui::SliderFloat("##c1", &m_active_scene->m_directional_light.cascade_ranges[0], 0.f, 50.f);
			ImGui::SliderFloat("##c2", &m_active_scene->m_directional_light.cascade_ranges[1], 0.f, 150.f);
			ImGui::SliderFloat("##c3", &m_active_scene->m_directional_light.cascade_ranges[2], 0.f, 500.f);
			ImGui::Text("Z-mults");
			ImGui::SliderFloat("##z1", &m_active_scene->m_directional_light.z_mults[0], 0.f, 10.f);
			ImGui::SliderFloat("##z2", &m_active_scene->m_directional_light.z_mults[1], 0.f, 10.f);
			ImGui::SliderFloat("##z3", &m_active_scene->m_directional_light.z_mults[2], 0.f, 10.f);

			ImGui::SliderFloat("Size", &m_active_scene->m_directional_light.light_size, 0.f, 150.f);
			ImGui::SliderFloat("Blocker search size", &m_active_scene->m_directional_light.blocker_search_size, 0.f, 50.f);

			m_active_scene->m_directional_light.color = glm::vec3(light_color.x, light_color.y, light_color.z);
			m_active_scene->m_directional_light.SetLightDirection(light_dir);
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
		ShowColorVec3Editor("Color", light->color);
	}




	void EditorLayer::RenderCameraEditor(CameraComponent* p_cam) {
		ImGui::PushItemWidth(200.f);
		ImGui::SliderFloat("Exposure", &p_cam->exposure, 0.f, 10.f);
		ImGui::SliderFloat("FOV", &p_cam->fov, 0.f, 180.f);
		ImGui::InputFloat("ZNEAR", &p_cam->zNear);
		ImGui::InputFloat("ZFAR", &p_cam->zFar);
		ShowVec3Editor("Up", p_cam->up);

		if (ImGui::Button("Make active")) {
			p_cam->MakeActive();
		}
	}




	bool EditorLayer::ShowVec3Editor(const char* name, glm::vec3& vec, float min, float max) {
		bool ret = false;
		glm::vec3 vec_copy = vec;
		ImGui::PushID(&vec);
		ImGui::Text(name);
		ImGui::PushItemWidth(100.f);
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "X");
		ImGui::SameLine();

		if (ImGui::InputFloat("##x", &vec_copy.x) && vec_copy.x > min && vec_copy.x < max) {
			vec.x = vec_copy.x;
			ret = true;
		}


		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y");
		ImGui::SameLine();

		if (ImGui::InputFloat("##y", &vec_copy.y) && vec_copy.y > min && vec_copy.y < max) {
			vec.y = vec_copy.y;
			ret = true;
		}

		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 0, 1, 1), "Z");
		ImGui::SameLine();

		if (ImGui::InputFloat("##z", &vec_copy.z) && vec_copy.z > min && vec_copy.z < max) {
			vec.z = vec_copy.z;
			ret = true;
		}

		ImGui::PopItemWidth();
		ImGui::PopID();

		return ret;
	}




	bool EditorLayer::ShowVec2Editor(const char* name, glm::vec2& vec, float min, float max) {
		bool ret = false;
		glm::vec2 vec_copy = vec;
		ImGui::PushID(&vec);
		ImGui::Text(name);
		ImGui::PushItemWidth(100.f);
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "X");
		ImGui::SameLine();

		if (ImGui::InputFloat("##x", &vec_copy.x) && vec_copy.x > min && vec_copy.x < max) {
			vec.x = vec_copy.x;
			ret = true;
		}


		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y");
		ImGui::SameLine();

		if (ImGui::InputFloat("##y", &vec_copy.y) && vec_copy.y > min && vec_copy.y < max) {
			vec.y = vec_copy.y;
			ret = true;
		}

		ImGui::PopItemWidth();
		ImGui::PopID();

		return ret;
	}




	void EditorLayer::ShowFileExplorer(const std::string& starting_path, wchar_t extension_filter[], std::function<void(std::string)> valid_file_callback) {
		// Create an OPENFILENAMEW structure
		OPENFILENAMEW ofn;
		wchar_t fileNames[MAX_PATH * 100] = { 0 };

		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFile = fileNames;
		ofn.lpstrFilter = extension_filter;
		ofn.nMaxFile = sizeof(fileNames);
		ofn.Flags = OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_PATHMUSTEXIST;

		// This needs to be stored to keep relative filepaths working, otherwise the working directory will be changed
		std::filesystem::path prev_path{std::filesystem::current_path().generic_string()};
		// Display the File Open dialog
		if (GetOpenFileNameW(&ofn))
		{
			// Process the selected files
			std::wstring folderPath = fileNames;

			// Get the length of the folder path
			size_t folderPathLen = folderPath.length();

			// Pointer to the first character after the folder path
			wchar_t* currentFileName = fileNames + folderPathLen + 1;
			bool single_file = currentFileName[0] == '\0';

			int max_safety_iterations = 5000;
			int i = 0;
			// Loop through the selected files
			while (i < max_safety_iterations && (*currentFileName || single_file))
			{
				i++;
				// Construct the full file path
				std::wstring filePath = single_file ? folderPath : folderPath + L"\\" + currentFileName;

				std::string path_name = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(filePath);
				std::ranges::for_each(path_name, [](char& c) { c = c == '\\' ? '/' : c; });


				// Reset path to stop relative paths breaking
				std::filesystem::current_path(prev_path);
				if (path_name.size() <= ORNG_MAX_FILEPATH_SIZE)
					valid_file_callback(path_name);
				else
					ORNG_CORE_ERROR("Path name '{0}' exceeds maximum path length limit : {1}", path_name, ORNG_MAX_FILEPATH_SIZE);

				// Move to the next file name
				currentFileName += wcslen(currentFileName) + 1;
				single_file = false;
			}



		}

	}




	bool EditorLayer::ShowColorVec3Editor(const char* name, glm::vec3& vec) {
		bool ret = false;
		ImGui::PushID(&vec);
		ImGui::Text(name);
		ImGui::PushItemWidth(100.f);

		ImGui::TextColored(ImVec4(1, 0, 0, 1), "R");
		ImGui::SameLine();

		if (ImGui::InputFloat("##r", &vec.x))
			ret = true;

		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "G");
		ImGui::SameLine();

		if (ImGui::InputFloat("##g", &vec.y))
			ret = true;

		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 0, 1, 1), "B");
		ImGui::SameLine();

		if (ImGui::InputFloat("##b", &vec.z))
			ret = true;

		ImGui::PopItemWidth();
		ImGui::PopID();

		return ret;
	}




	void EditorLayer::RenderErrorMessages() {
		ImVec2 window_size{ 750, 500 };
		ImGui::SetNextWindowSize(window_size);
		ImGui::SetNextWindowPos(ImVec2((Window::GetWidth() - window_size.x) / 2.f, (Window::GetHeight() - window_size.y) / 2.f));
		for (int i = m_error_log_stack.size() - 1; i >= 0; i--) {
			ImGui::PushID(&m_error_log_stack[i]);

			if (ImGui::Begin("Error window")) {

				ImGui::PushFont(mp_large_font);
				ImGui::SeparatorText("Error");
				ImGui::SameLine(ImGui::GetContentRegionAvail().x - 50);

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8, 0, 0, 1));
				if (ImGui::IsWindowHovered && ImGui::IsMouseDoubleClicked(1) || ImGui::Button("X")) {
					m_error_log_stack.erase(m_error_log_stack.begin() + i);
					ImGui::PopFont();
					ImGui::PopStyleColor();
					ImGui::End();
					ImGui::PopID();
					continue;
				}
				ImGui::PopStyleColor();

				ImGui::PopFont();
				ImGui::SeparatorText("Recent logs");

				for (int y = m_error_log_stack[i].size() - 1; y >= 0; y--) {
					ImGui::Text(m_error_log_stack[i][y].c_str());
				}

			}

			ImGui::End();
			ImGui::PopID();
		}

	}

	void EditorLayer::GenerateErrorMessage(const std::string& error_str) {
		auto& str_vec = m_error_log_stack.emplace_back(Log::GetLastLogs());
		str_vec.push_back(error_str);
	}

	bool EditorLayer::ClampedFloatInput(const char* name, float* p_val, float min, float max) {
		float val = *p_val;
		bool r = false;
		if (ImGui::InputFloat(name, &val) && val <= max && val >= min) {
			*p_val = val;
			r = true;
		}
		return r;
	}

	bool EditorLayer::H1TreeNode(const char* name) {
		float original_size = ImGui::GetFont()->Scale;
		ImGui::GetFont()->Scale *= 1.15f;
		ImGui::PushFont(ImGui::GetFont());
		bool r = ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen);
		ImGui::GetFont()->Scale = original_size;
		ImGui::PopFont();
		return r;
	}

	bool EditorLayer::H2TreeNode(const char* name) {
		float original_size = ImGui::GetFont()->Scale;
		ImGui::GetFont()->Scale *= 1.1f;
		ImGui::PushFont(ImGui::GetFont());
		bool r = ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen);
		ImGui::GetFont()->Scale = original_size;
		ImGui::PopFont();
		return r;
	}

	bool EditorLayer::EmptyTreeNode(const char* name) {
		bool ret = ImGui::TreeNode(name);
		if (ret)
			ImGui::TreePop();

		return ret;
	}

}