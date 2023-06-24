#include "pch/pch.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include "../extern/Icons.h"
#include "fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD.h"
#include "layers/EditorLayer.h"
#include "rendering/Renderer.h"
#include "scene/Scene.h"
#include "scene/MeshInstanceGroup.h"
#include "components/lights/PointLightComponent.h"
#include "components/lights/DirectionalLight.h"
#include "components/lights/SpotLightComponent.h"
#include "components/ScriptComponent.h"
#include "scene/SceneEntity.h"
#include "components/MeshComponent.h"
#include "util/Log.h"
#include "rendering/SceneRenderer.h"
#include "../extern/imguizmo/ImGuizmo.h"
#include "core/GLStateManager.h"
#include "core/Window.h"
#include "core/FrameTiming.h"

namespace ORNG {

	void EditorLayer::Init() {
		InitImGui();
		m_active_scene = std::make_unique<Scene>();
		m_active_scene->mp_active_camera = &m_editor_camera;

		m_grid_mesh = std::make_unique<GridMesh>();
		m_grid_mesh->Init();
		mp_grid_shader = &Renderer::GetShaderLibrary().CreateShader("grid");
		mp_grid_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/GridVS.glsl");
		mp_grid_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/GridFS.glsl");
		mp_grid_shader->Init();

		mp_quad_shader = &Renderer::GetShaderLibrary().CreateShader("2d_quad");
		mp_quad_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/QuadVS.glsl");
		mp_quad_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/QuadFS.glsl");
		mp_quad_shader->Init();

		mp_picking_shader = &Renderer::GetShaderLibrary().CreateShader("picking");
		mp_picking_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/TransformVS.glsl");
		mp_picking_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/PickingFS.glsl");
		mp_picking_shader->Init();
		mp_picking_shader->AddUniform("comp_id");
		mp_picking_shader->AddUniform("transform");

		mp_highlight_shader = &Renderer::GetShaderLibrary().CreateShader("highlight");
		mp_highlight_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/TransformVS.glsl");
		mp_highlight_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/HighlightFS.glsl");
		mp_highlight_shader->Init();
		mp_highlight_shader->AddUniform("transform");


		Texture2DSpec picking_spec;
		picking_spec.format = GL_RED_INTEGER;
		picking_spec.internal_format = GL_R32UI;
		picking_spec.storage_type = GL_UNSIGNED_INT;
		picking_spec.width = Window::GetWidth();
		picking_spec.height = Window::GetHeight();


		mp_picking_fb = &Renderer::GetFramebufferLibrary().CreateFramebuffer("picking", true);
		mp_picking_fb->AddRenderbuffer(Window::GetWidth(), Window::GetHeight());
		mp_picking_fb->Add2DTexture("component_ids", GL_COLOR_ATTACHMENT0, picking_spec);

		SceneRenderer::SetActiveScene(&*m_active_scene);


		Texture2DSpec color_render_texture_spec;
		color_render_texture_spec.format = GL_RGB;
		color_render_texture_spec.internal_format = GL_RGB16F;
		color_render_texture_spec.storage_type = GL_FLOAT;
		color_render_texture_spec.width = Window::GetWidth();
		color_render_texture_spec.height = Window::GetHeight();


		mp_editor_pass_fb = &Renderer::GetFramebufferLibrary().CreateFramebuffer("editor_passes", true);
		mp_editor_pass_fb->AddShared2DTexture("shared_depth", Renderer::GetFramebufferLibrary().GetFramebuffer("gbuffer").GetTexture<Texture2D>("shared_depth"), GL_DEPTH_ATTACHMENT);
		mp_editor_pass_fb->AddShared2DTexture("shared_render_texture", Renderer::GetFramebufferLibrary().GetFramebuffer("post_processing").GetTexture<Texture2D>("shared_render_texture"), GL_COLOR_ATTACHMENT0);



		m_active_scene->LoadScene();
		Texture2D* p_orange_tex = m_active_scene->CreateTexture2DAsset("./res/meshes/oranges/orangeC.jpg", true);
		for (float x = 0; x <= 100.f; x += 10.f) {
			auto& t_entity = m_active_scene->CreateEntity("Orange");
			auto* t_mesh = t_entity.AddComponent<MeshComponent>("./res/meshes/oranges/orange.obj");
			auto* p_material = m_active_scene->CreateMaterial();

			t_mesh->transform.SetPosition(x, 20.f, 0.f);
			p_material->roughness = x / 100.f + 0.05f;
			p_material->base_color_texture = m_active_scene->GetMaterial(0)->base_color_texture;
			p_material->metallic = 0.05;
			p_material->base_color = glm::vec3(0.0, 1.0, 1.0);
			t_mesh->SetMaterialID(0, p_material);
			t_mesh->SetMaterialID(1, p_material);
		}

		for (float x = 0; x <= 100.f; x += 10.f) {
			auto& t_entity = m_active_scene->CreateEntity("Orange");
			auto* t_mesh = t_entity.AddComponent<MeshComponent>("./res/meshes/oranges/orange.obj");
			auto* p_material = m_active_scene->CreateMaterial();

			t_mesh->transform.SetPosition(x, 30.f, 0.f);
			p_material->base_color_texture = m_active_scene->GetMaterial(0)->base_color_texture;
			p_material->ao = 1.0;
			p_material->metallic = 1.0 - (x / 100.f) + 0.05f;
			p_material->roughness = 0.05;
			p_material->base_color = glm::vec3(0.0, 1.0, 1.0);
			t_mesh->SetMaterialID(0, p_material);
			t_mesh->SetMaterialID(1, p_material);
		}

		auto& t_entity = m_active_scene->CreateEntity("Orange");
		auto* t_mesh = t_entity.AddComponent<MeshComponent>("./res/meshes/balloon/air balloon.obj");
		auto p_script = t_entity.AddComponent<ScriptComponent>();
		p_script->OnUpdate = [t_mesh] {
			if (Window::IsKeyDown('w'))
				t_mesh->transform.SetPosition(t_mesh->transform.GetPosition() + glm::vec3(0, 1, 0) * sinf(FrameTiming::GetTimeStep()));
		};


		OAR_CORE_INFO("Editor layer initialized"); //add profiling func
	}

	void EditorLayer::InitImGui() {
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.Fonts->AddFontDefault();
		io.FontDefault = io.Fonts->AddFontFromFileTTF("./res/fonts/PlatNomor-WyVnn.ttf", 18.0f);

		ImFontConfig config;
		config.MergeMode = true;
		//io.FontDefault = io.Fonts->AddFontFromFileTTF("./res/fonts/PlatNomor-WyVnn.ttf", 18.0f);
		static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		io.Fonts->AddFontFromFileTTF("./res/fonts/fa-regular-400.ttf", 18.0f, &config, icon_ranges);
		io.Fonts->AddFontFromFileTTF("./res/fonts/fa-solid-900.ttf", 18.0f, &config, icon_ranges);
		ImGui_ImplOpenGL3_CreateFontsTexture();
		ImGui::StyleColorsDark();



		ImGui::GetStyle().Colors[ImGuiCol_Button] = lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_Border] = dark_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_Tab] = lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_TabHovered] = lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_TabActive] = orange_color;
		ImGui::GetStyle().Colors[ImGuiCol_Header] = lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_HeaderHovered] = lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_HeaderActive] = orange_color;
		ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = dark_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_FrameBgHovered] = lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_FrameBgActive] = lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_TitleBg] = lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive] = lighter_grey_color;
	}


	void EditorLayer::Update() {

		if (!ImGui::GetIO().WantTextInput) {
			m_editor_camera.Update();
		}

		if (Window::IsKeyDown('K'))
			m_active_scene->MakeCameraActive(static_cast<CameraComponent*>(&m_editor_camera));

		m_active_scene->Update();
	}




	void EditorLayer::RenderUI() {

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(400, Window::GetHeight()));
		ImGui::Begin("Tools", (bool*)0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Text(std::format("Draw calls: {}", Renderer::Get().m_draw_call_amount).c_str());
		Renderer::ResetDrawCallCounter();

		RenderSceneGraph();

		ImGui::End();

		ShowAssetManager();

		RenderEditorWindow();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	}



	void EditorLayer::RenderDisplayWindow() {

		Renderer::GetFramebufferLibrary().UnbindAllFramebuffers();
		if (Window::IsMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT) && !ImGui::GetIO().WantCaptureMouse) {
			DoPickingPass();
		}
		GL_StateManager::DefaultClearBits();

		SceneRenderer::SceneRenderingSettings settings;
		SceneRenderer::SceneRenderingOutput output = SceneRenderer::RenderScene(settings);

		mp_editor_pass_fb->Bind();
		DoSelectedEntityHighlightPass();
		RenderGrid();

		Renderer::GetFramebufferLibrary().UnbindAllFramebuffers();
		mp_quad_shader->ActivateProgram();
		GL_StateManager::BindTexture(GL_TEXTURE_2D, output.final_color_texture_handle, GL_StateManager::TextureUnits::COLOR);

		// Render scene texture to editor display window (currently just fullscreen quad)
		glDisable(GL_DEPTH_TEST);
		Renderer::DrawQuad();
		glEnable(GL_DEPTH_TEST);

	}


	void EditorLayer::RenderEditorWindow() {
		ImGui::SetNextWindowPos(ImVec2(Window::GetWidth() - 400, 0));
		ImGui::SetNextWindowSize(ImVec2(400, Window::GetHeight()));

		if (ImGui::Begin("Editor", (bool*)0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
			DisplayEntityEditor();

			if (mp_selected_material)
				RenderMaterialEditorSection();

			if (mp_selected_texture)
				RenderTextureEditorSection();

			ImGui::End();
		}
	}

	void EditorLayer::DoPickingPass() {

		mp_picking_fb->Bind();
		mp_picking_shader->ActivateProgram();

		GL_StateManager::ClearDepthBits();
		GL_StateManager::ClearBitsUnsignedInt();

		for (auto& mesh : m_active_scene->m_mesh_component_manager.GetMeshComponents()) {
			if (!mesh || !mesh->GetMeshData()) continue;

			if (mesh->GetMeshData()->GetLoadStatus() == true) {
				mp_picking_shader->SetUniform<unsigned int>("comp_id", mesh->GetEntityHandle());
				mp_picking_shader->SetUniform("transform", mesh->transform.GetMatrix());

				for (int i = 0; i < mesh->mp_mesh_asset->m_submeshes.size(); i++) {
					Renderer::DrawSubMesh(mesh->GetMeshData(), i);
				}

			}
		}

		glm::vec2 mouse_coords = glm::min(glm::max(Window::GetMousePos(), glm::vec2(1, 1)), glm::vec2(Window::GetWidth() - 1, Window::GetHeight() - 1));

		GLuint* pixels = new GLuint[1];
		glReadPixels(mouse_coords.x, Window::GetHeight() - mouse_coords.y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, pixels);
		unsigned int current_entity_id = pixels[0];
		delete[] pixels;
		m_selected_entity_id = current_entity_id;
	}




	void EditorLayer::DoSelectedEntityHighlightPass() {
		auto* current_entity = m_active_scene->GetEntity(m_selected_entity_id);

		if (!current_entity)
			return;

		MeshComponent* meshc = current_entity->GetComponent<MeshComponent>();

		if (!meshc)
			return;

		mp_editor_pass_fb->Bind();
		mp_highlight_shader->ActivateProgram();
		mp_highlight_shader->SetUniform("transform", meshc->transform.GetMatrix());

		for (int i = 0; i < meshc->GetMeshData()->m_submeshes.size(); i++) {
			Renderer::DrawSubMesh(meshc->GetMeshData(), i);
		}

	}


	void EditorLayer::RenderGlobalFogEditor() {
		if (ImGui::TreeNode("Global fog")) {
			ImGui::Text("Scattering");
			ImGui::SliderFloat("##scattering", &m_active_scene->m_global_fog.scattering_coef, 0.f, 0.1f);
			ImGui::Text("Absorption");
			ImGui::SliderFloat("##absorption", &m_active_scene->m_global_fog.absorption_coef, 0.f, 0.1f);
			ImGui::Text("Density");
			ImGui::SliderFloat("##density", &m_active_scene->m_global_fog.density_coef, 0.f, 1.f);
			ImGui::Text("Scattering anistropy");
			ImGui::SliderFloat("##scattering anistropy", &m_active_scene->m_global_fog.scattering_anistropy, -1.f, 1.f);
			ImGui::Text("Step count");
			ImGui::SliderInt("##step count", &m_active_scene->m_global_fog.step_count, 0, 512);
			ShowColorVec3Editor("Color", m_active_scene->m_global_fog.color);
			ImGui::TreePop();
		}
	}




	void EditorLayer::RenderGrid() {
		GL_StateManager::BindSSBO(m_grid_mesh->m_ssbo_handle, GL_StateManager::SSBO_BindingPoints::TRANSFORMS);
		m_grid_mesh->CheckBoundary(m_editor_camera.pos);
		mp_grid_shader->ActivateProgram();
		Renderer::DrawVAO_ArraysInstanced(GL_LINES, m_grid_mesh->m_vao, ceil(m_grid_mesh->grid_width / m_grid_mesh->grid_step) * 2);
	}




	void EditorLayer::RenderTerrainEditor() {
		if (ImGui::TreeNode("Terrain")) {
			ImGui::InputFloat("Height factor", &m_active_scene->m_terrain.m_height_scale);
			static int terrain_width = 1000;

			if (ImGui::InputInt("Size", &terrain_width) && terrain_width > 500)
				m_active_scene->m_terrain.m_width = terrain_width;
			else
				terrain_width = m_active_scene->m_terrain.m_width;

			static int terrain_seed = 123;
			ImGui::InputInt("Seed", &terrain_seed);
			m_active_scene->m_terrain.m_seed = terrain_seed;

			if (ImGui::Button("Reload"))
				m_active_scene->m_terrain.ResetTerrainQuadtree();

			ImGui::TreePop();
		}
	}


	void EditorLayer::ShowAssetManager() {


		static std::string error_message = "";
		static std::string path = "./res/meshes";
		static std::string entry_name = "";

		int window_height = glm::clamp(static_cast<int>(Window::GetHeight()) / 4, 100, 500);
		int window_width = Window::GetWidth() - 800;
		ImVec2 button_size = { glm::clamp(window_width / 8.f, 75.f, 150.f) , 150 };
		int column_count = 6;
		ImGui::SetNextWindowSize(ImVec2(window_width, window_height));
		ImGui::SetNextWindowPos(ImVec2(400, Window::GetHeight() - window_height));
		ImGui::Begin("Assets", (bool*)0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
		ImGui::BeginTabBar("Selection");
		ImGui::GetStyle().CellPadding = ImVec2(11.f, 15.f);


		if (ImGui::BeginTabItem("Meshes")) // MESH TAB
		{
			if (ImGui::TreeNode("Add mesh")) // MESH FILE EXPLORER
			{
				// error message display
				ImGui::TextColored(ImVec4(1, 0, 0, 1), error_message.c_str());

				static std::string file_extension = "";
				static std::vector<std::string> mesh_file_extensions = { ".OBJ", ".FBX" };

				//setting up file explorer callbacks
				std::function<void()> success_callback = [this] {
					MeshAsset* asset = m_active_scene->CreateMeshAsset(path + "/" + entry_name);
					asset->LoadMeshData();
					m_active_scene->LoadMeshAssetIntoGPU(asset);
					error_message.clear();
				};

				std::function<void()> error_callback = [] {
					file_extension = entry_name.substr(entry_name.find_last_of("."));
					error_message = "Invalid file type: " + file_extension;
				};

				ShowFileExplorer(path, entry_name, mesh_file_extensions, success_callback, error_callback);

				ImGui::TreePop();
			} // END MESH FILE EXPLORER


			if (ImGui::BeginTable("Meshes", 4, ImGuiTableFlags_Borders)) // MESH VIEWING TABLE
			{
				for (auto& data : m_active_scene->m_mesh_assets)
				{
					ImGui::PushID(data);
					ImGui::TableNextColumn();
					ImGui::Text(data->GetFilename().substr(data->GetFilename().find_last_of('/') + 1).c_str());
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 0, 0, 0.5));

					if (ImGui::SmallButton("X"))
					{
						m_active_scene->DeleteMeshAsset(data);
					}

					ImGui::PopStyleColor();
					if (data->is_loaded)
						ImGui::TextColored(ImVec4(0, 1, 0, 1), "LOADED");
					else
						ImGui::TextColored(ImVec4(1, 0, 0, 1), "NOT LOADED");

					ImGui::PopID();
				}

				ImGui::EndTable();
				ImGui::EndTabItem();

			} // END MESH VIEWING TABLE
		} //	 END MESH TAB


		if (ImGui::BeginTabItem("Textures")) // TEXTURE TAB
		{

			if (ImGui::TreeNode("Add texture")) {
				ImGui::TextColored(ImVec4(1, 0, 0, 1), error_message.c_str());

				static std::string file_extension = "";
				static std::vector<std::string> valid_filepaths = { ".PNG", ".JPG", ".JPEG", ".TIF" };

				//setting up file explorer callbacks
				static std::function<void()> success_callback = [this] {
					m_active_scene->CreateTexture2DAsset(path + "/" + entry_name, m_current_2d_tex_spec.srgb_space);
					error_message.clear();
				};

				static std::function<void()> fail_callback = [] {
					error_message = file_extension.empty() ? "" : "Invalid file type: " + file_extension; // if file extension is empty it's a directory, so no error
				};

				ShowFileExplorer(path, entry_name, valid_filepaths, success_callback, fail_callback);

				ImGui::TreePop();
			}


			if (ImGui::TreeNode("Texture viewer")) // TEXTURE VIEWING TREE NODE
			{

				// Create table for textures 
				if (ImGui::BeginTable("Textures", column_count, ImGuiTableFlags_Borders | ImGuiTableFlags_PadOuterX)); // TEXTURE VIEWING TABLE
				{
					// Push textures into table 
					for (auto* p_texture : m_active_scene->m_texture_2d_assets)
					{
						ImGui::PushID(p_texture);
						ImGui::TableNextColumn();

						if (ImGui::ImageButton(ImTextureID(p_texture->GetTextureHandle()), button_size)) {
							mp_selected_texture = p_texture;
							m_current_2d_tex_spec = mp_selected_texture->m_spec;
						};

						if (ImGui::IsItemActivated()) {
							mp_dragged_texture = p_texture;
						}


						ImGui::Text(p_texture->m_spec.filepath.substr(p_texture->m_spec.filepath.find_last_of('/') + 1).c_str());

						ImGui::PopID();

					}

					ImGui::EndTable();
				} // END TEXTURE VIEWING TABLE
				ImGui::TreePop();
			}
			ImGui::EndTabItem();
		} // END TEXTURE TAB



		if (ImGui::BeginTabItem("Materials")) { // MATERIAL TAB

			if (ImGui::BeginTable("Material viewer", column_count, ImGuiTableFlags_Borders | ImGuiTableFlags_PadOuterX, ImVec2(window_width, window_height))) { //MATERIAL VIEWING TABLE

				for (auto* p_material : m_active_scene->m_materials) {
					ImGui::TableNextColumn();

					ImGui::PushID(p_material);

					if (ImGui::ImageButton(ImTextureID(p_material->base_color_texture->m_texture_obj), button_size))
						mp_selected_material = p_material;

					if (ImGui::IsItemActivated()) {
						mp_dragged_material = p_material;
					}

					ImGui::Text(p_material->name.c_str());

					ImGui::PopID();
				}


				ImGui::EndTable();
			} //END MATERIAL VIEWING TABLE
			ImGui::EndTabItem();
		} //END MATERIAL TAB


		ImGui::GetStyle().CellPadding = ImVec2(4.f, 4.f);
		ImGui::EndTabBar();
		ImGui::End();
	}


	void EditorLayer::RenderTextureEditorSection() {

		if (H1TreeNode("Texture editor")) {
			ImGui::SameLine();
			if (ImGui::Button("X")) {
				mp_selected_texture = nullptr;
				ImGui::TreePop();
				return;
			}

			ImGui::Text(mp_selected_texture->GetSpec().filepath.c_str());

			const char* wrap_modes[] = { "REPEAT", "CLAMP TO EDGE" };
			const char* filter_modes[] = { "NEAREST", "LINEAR" };
			static int selected_wrap_mode = m_current_2d_tex_spec.wrap_params == GL_REPEAT ? 0 : 1;
			static int selected_filter_mode = m_current_2d_tex_spec.mag_filter == GL_NEAREST ? 0 : 1;


			ImGui::Text("Wrap mode");
			ImGui::SameLine();
			ImGui::Combo("##Wrap mode", &selected_wrap_mode, wrap_modes, IM_ARRAYSIZE(wrap_modes));
			m_current_2d_tex_spec.wrap_params = selected_wrap_mode == 0 ? GL_REPEAT : GL_CLAMP_TO_EDGE;

			ImGui::Text("Filtering");
			ImGui::SameLine();
			ImGui::Combo("##Filter mode", &selected_filter_mode, filter_modes, IM_ARRAYSIZE(filter_modes));
			m_current_2d_tex_spec.mag_filter = selected_filter_mode == 0 ? GL_NEAREST : GL_LINEAR;
			m_current_2d_tex_spec.min_filter = selected_filter_mode == 0 ? GL_NEAREST : GL_LINEAR;

			if (ImGui::Button("Load")) {
				mp_selected_texture->SetSpec(m_current_2d_tex_spec);
				mp_selected_texture->LoadFromFile();
			}
			ImGui::TreePop();
		}
	}



	void EditorLayer::RenderMaterialEditorSection() {

		if (H1TreeNode("Material editor")) {
			ImGui::SameLine();


			if (ImGui::SmallButton("X")) {
				mp_selected_material = nullptr;
				ImGui::TreePop();
				return;
			}

			ImGui::Text("Name: ");
			ImGui::SameLine();
			ImGui::InputText("##name input", &mp_selected_material->name);
			ImGui::Spacing();

			RenderMaterialTexture("Base", mp_selected_material->base_color_texture);
			RenderMaterialTexture("Normal", mp_selected_material->normal_map_texture);
			RenderMaterialTexture("Roughness", mp_selected_material->roughness_texture);
			RenderMaterialTexture("Metallic", mp_selected_material->metallic_texture);
			RenderMaterialTexture("Ambient occlusion", mp_selected_material->ao_texture);
			RenderMaterialTexture("Displacement", mp_selected_material->displacement_texture);



			ImGui::Text("Colors");
			ImGui::Spacing();
			ShowVec3Editor("Base color", mp_selected_material->base_color);

			if (!mp_selected_material->roughness_texture)
				ImGui::SliderFloat("Roughness", &mp_selected_material->roughness, 0.f, 1.f);

			if (!mp_selected_material->metallic_texture)
				ImGui::SliderFloat("Metallic", &mp_selected_material->metallic, 0.f, 1.f);

			int num_parallax_layers = mp_selected_material->parallax_layers;
			if (mp_selected_material->displacement_texture) {
				ImGui::InputInt("Parallax layers", &num_parallax_layers);

				if (num_parallax_layers >= 0)
					mp_selected_material->parallax_layers = num_parallax_layers;

				ImGui::InputFloat("Parallax scale", &mp_selected_material->parallax_height_scale);
			}


			ImGui::TreePop();
		}

		if (!Window::IsMouseButtonDown(GLFW_MOUSE_BUTTON_1))
			// Reset drag if mouse not held down
			mp_dragged_texture = nullptr;

	}





	void EditorLayer::RenderSceneGraph() {
		if (ImGui::CollapsingHeader("Scene")) {
			ImGui::SliderFloat("Exposure", &m_active_scene->m_exposure_level, 0.f, 10.f);
			RenderDirectionalLightEditor();
			RenderGlobalFogEditor();
			RenderTerrainEditor();
			if (ImGui::TreeNode("Entities")) {
				for (auto& entity : m_active_scene->m_entities) {
					if (entity && ImGui::Button(entity->name, ImVec2(200, 25))) {
						m_selected_entity_id = entity->GetID();
					}
				}
				ImGui::TreePop();
			}
		}
	}




	void EditorLayer::DisplayEntityEditor() {
		auto entity = m_active_scene->GetEntity(m_selected_entity_id);
		if (!entity) return;

		auto meshc = entity->GetComponent<MeshComponent>();
		auto plight = entity->GetComponent<PointLightComponent>();
		auto slight = entity->GetComponent<SpotLightComponent>();
		auto p_cam = entity->GetComponent<CameraComponent>();
		auto p_transform = entity->GetComponent<TransformComponent>();


		if (H1TreeNode("Entity editor")) {
			std::string ent_text = std::format("Entity '{}'", entity->name);
			ImGui::Text(ent_text.c_str());


			//TRANSFORM
			if (H2TreeNode("Entity transform")) {
				RenderTransformComponentEditor(p_transform);
				ImGui::TreePop();
			}


			//MESH
			if (meshc) {

				if (H2TreeNode("Mesh component")) {
					ImGui::Text("Mesh asset name");
					ImGui::SameLine();
					static std::string input_filename;
					ImGui::InputText("##filename input", &input_filename);

					if (ImGui::Button("Link asset")) {

						for (auto p_mesh_asset : m_active_scene->m_mesh_assets) {
							std::string mesh_filename = p_mesh_asset->GetFilename().substr(p_mesh_asset->GetFilename().find_last_of('/') + 1);
							std::transform(mesh_filename.begin(), mesh_filename.end(), mesh_filename.begin(), ::toupper);
							std::transform(input_filename.begin(), input_filename.end(), input_filename.begin(), ::toupper);

							if (mesh_filename == input_filename) {
								meshc->SetMeshAsset(p_mesh_asset);
							}

						}
					};

					if (meshc->mp_mesh_asset) {

						RenderMeshComponentEditor(meshc);
					}
					ImGui::TreePop();
				}

			}


			ImGui::PushID(plight);
			if (plight) {
				if (H2TreeNode("Pointlight component")) {
					ImGui::SameLine();
					if (ImGui::Button("X", ImVec2(25, 25))) {
						entity->DeleteComponent<PointLightComponent>();
					};
					RenderPointlightEditor(plight);
					ImGui::TreePop();
				}
			}
			else {
				ImGui::Text("Pointlight");
				ImGui::SameLine();
				if (ImGui::Button("+", ImVec2(25, 25))) {
					entity->AddComponent<PointLightComponent>();
				}
			}
			ImGui::PopID();

			ImGui::PushID(slight);
			if (slight) {
				if (H2TreeNode("Spotlight component")) {
					if (ImGui::Button("X", ImVec2(25, 25))) {
						entity->DeleteComponent<SpotLightComponent>();
					};
					RenderSpotlightEditor(slight);
					ImGui::TreePop();
				}
			}
			else {
				ImGui::Text("Spotlight");
				ImGui::SameLine();
				if (ImGui::Button("+", ImVec2(25, 25))) {
					entity->AddComponent<SpotLightComponent>();
				}
			}
			ImGui::PopID();

			ImGui::PushID(p_cam);
			if (p_cam) {
				if (H2TreeNode("Camera component")) {

					if (ImGui::Button("X", ImVec2(25, 25))) {
						entity->DeleteComponent<CameraComponent>();
					}

					RenderCameraEditor(p_cam);

					ImGui::TreePop();
				};
			}
			ImGui::PopID();
			ImGui::TreePop();
		}
	}



	// EDITORS ------------------------------------------------------------------------

	void EditorLayer::RenderTransformComponentEditor(TransformComponent* p_transform) {


		static TransformComponent* p_active_guizmo_transform = nullptr;
		bool render_gizmos = p_active_guizmo_transform == p_transform;

		if (ImGui::RadioButton("Gizmos", render_gizmos))
			p_active_guizmo_transform = render_gizmos ? nullptr : p_transform;

		ImGui::SameLine();

		if (ImGui::RadioButton("Absolute", p_transform->m_is_absolute)) {
			p_transform->SetAbsoluteMode(!p_transform->m_is_absolute);
		}


		glm::vec3 matrix_translation = p_transform->m_pos;
		glm::vec3 matrix_rotation = p_transform->m_rotation;
		glm::vec3 matrix_scale = p_transform->m_scale;


		if (ShowVec3Editor("Tr", matrix_translation))
			p_transform->SetPosition(matrix_translation);

		if (ShowVec3Editor("Rt", matrix_rotation))
			p_transform->SetOrientation(matrix_rotation);

		if (ShowVec3Editor("Sc", matrix_scale))
			p_transform->SetScale(matrix_scale);

		if (p_active_guizmo_transform != p_transform) // Second check here as render_gizmos is no longer accurate due to potential change
			return;


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

		glm::mat4 current_operation_matrix = p_transform->GetMatrix();


		if (ImGuizmo::Manipulate(&m_editor_camera.GetViewMatrix()[0][0], &m_editor_camera.GetProjectionMatrix()[0][0], current_operation, current_mode, &current_operation_matrix[0][0], nullptr, nullptr) && ImGuizmo::IsUsing()) {

			ImGuizmo::DecomposeMatrixToComponents(&current_operation_matrix[0][0], &matrix_translation[0], &matrix_rotation[0], &matrix_scale[0]);
			auto abs_transforms = p_transform->GetAbsoluteTransforms();
			glm::vec3 abs_translation = abs_transforms[0];
			glm::vec3 abs_scale = abs_transforms[1];
			glm::vec3 abs_rotation = abs_transforms[2];

			switch (current_operation) {
			case ImGuizmo::TRANSLATE:
				glm::vec3 new_pos = p_transform->m_is_absolute ? matrix_translation : matrix_translation - (abs_translation - p_transform->GetPosition());
				p_transform->SetPosition(new_pos);
				break;
			case ImGuizmo::SCALE:
				glm::vec3 new_scale = p_transform->m_is_absolute ? matrix_scale : matrix_scale / (abs_scale / p_transform->GetScale());
				p_transform->SetScale(new_scale);
				break;
			case ImGuizmo::ROTATE:
				glm::vec3 new_orientation = p_transform->m_is_absolute ? matrix_rotation : matrix_rotation - (abs_rotation - p_transform->GetRotation());
				p_transform->SetOrientation(new_orientation);
				break;
			}
		};

	}



	void EditorLayer::RenderMeshComponentEditor(MeshComponent* comp) {

		static MeshComponentData mesh_data;

		ImGui::PushID(comp);

		for (int i = 0; i < comp->m_materials.size(); i++) {
			auto p_material = comp->m_materials[i];
			ImGui::PushID(i);
			if (ImGui::ImageButton(ImTextureID(p_material->base_color_texture->GetTextureHandle()), ImVec2(100, 100))) {
				mp_selected_material = m_active_scene->GetMaterial(p_material->material_id);
			};

			if (ImGui::IsItemHovered() && mp_dragged_material) {
				comp->SetMaterialID(i, mp_dragged_material);
				mp_dragged_material = nullptr;
			}

			ImGui::Text(p_material->name.c_str());
			ImGui::PopID();
		}

		if (H2TreeNode("Mesh transform")) {
			ImGui::Dummy(ImVec2(0.f, 20.f));
			RenderTransformComponentEditor(&comp->transform);
			ImGui::Dummy(ImVec2(0.f, 20.f));
			ImGui::TreePop();
		}

		ImGui::PopID();
	};

	void EditorLayer::RenderSpotlightEditor(SpotLightComponent* light) {
		static SpotLightConfigData light_data;

		light_data.aperture = glm::degrees(acosf(light->aperture));
		light_data.direction = light->m_light_direction_vec;
		light_data.base.atten_constant = light->attenuation.constant;
		light_data.base.atten_exp = light->attenuation.exp;
		light_data.base.atten_linear = light->attenuation.linear;
		light_data.base.color = light->color;
		light_data.base.max_distance = light->max_distance;
		light_data.base.pos = light->transform.GetPosition();

		ImGui::PushItemWidth(200.f);
		ImGui::SliderFloat("constant", &light_data.base.atten_constant, 0.0f, 1.0f);
		ImGui::SliderFloat("linear", &light_data.base.atten_linear, 0.0f, 1.0f);
		ImGui::SliderFloat("exp", &light_data.base.atten_exp, 0.0f, 0.1f);
		ImGui::SliderFloat("max distance", &light_data.base.max_distance, 0.0f, 5000.0f);

		ImGui::Text("Aperture");
		ImGui::SameLine();
		ImGui::SliderFloat("##a", &light_data.aperture, 0.f, 90.f);

		ImGui::PopItemWidth();

		ShowVec3Editor("Direction", light_data.direction);

		ShowColorVec3Editor("Color", light_data.base.color);
		ShowVec3Editor("Position", light_data.base.pos);

		light->attenuation.constant = light_data.base.atten_constant;
		light->attenuation.linear = light_data.base.atten_linear;
		light->attenuation.exp = light_data.base.atten_exp;
		light->max_distance = light_data.base.max_distance;
		light->color = light_data.base.color;
		light->transform.SetPosition(light_data.base.pos);
		light->SetLightDirection(light_data.direction.x, light_data.direction.y, light_data.direction.z);
		light->SetAperture(light_data.aperture);
		light->UpdateLightTransform();
	}


	void EditorLayer::RenderDirectionalLightEditor() {
		static DirectionalLightData light_data;

		if (ImGui::TreeNode("Directional light")) {
			ImGui::Text("DIR LIGHT CONTROLS");

			static glm::vec3 light_dir = glm::vec3(0, 0.5, 0.5);

			ImGui::SliderFloat("X", &light_data.light_direction.x, -1.f, 1.f);
			ImGui::SliderFloat("Y", &light_data.light_direction.y, -1.f, 1.f);
			ImGui::SliderFloat("Z", &light_data.light_direction.z, -1.f, 1.f);
			ShowColorVec3Editor("Color", light_data.light_color);

			m_active_scene->m_directional_light.color = glm::vec3(light_data.light_color.x, light_data.light_color.y, light_data.light_color.z);
			m_active_scene->m_directional_light.SetLightDirection(light_data.light_direction);
			ImGui::TreePop();
		}
	}


	void EditorLayer::RenderPointlightEditor(PointLightComponent* light) {
		static PointLightConfigData light_data;

		light_data.color = light->color;
		light_data.atten_exp = light->attenuation.exp;
		light_data.atten_linear = light->attenuation.linear;
		light_data.atten_constant = light->attenuation.constant;
		light_data.max_distance = light->max_distance;
		light_data.pos = light->transform.GetPosition();

		ImGui::PushItemWidth(200.f);
		ImGui::SliderFloat("constant", &light_data.atten_constant, 0.0f, 1.0f);
		ImGui::SliderFloat("linear", &light_data.atten_linear, 0.0f, 1.0f);
		ImGui::SliderFloat("exp", &light_data.atten_exp, 0.0f, 0.1f);
		ImGui::SliderFloat("max distance", &light_data.max_distance, 0.0f, 5000.0f);
		ImGui::PopItemWidth();
		ShowColorVec3Editor("Color", light_data.color);
		ShowVec3Editor("Position", light_data.pos);

		light->attenuation.constant = light_data.atten_constant;
		light->attenuation.linear = light_data.atten_linear;
		light->attenuation.exp = light_data.atten_exp;
		light->max_distance = light_data.max_distance;
		light->color = light_data.color;
		light->transform.SetPosition(light_data.pos);
	}

	void EditorLayer::RenderCameraEditor(CameraComponent* p_cam) {
		ImGui::PushItemWidth(200.f);
		ImGui::SliderFloat("FOV", &p_cam->fov, 0.f, 180.f);
		ImGui::InputFloat("ZNEAR", &p_cam->zNear);
		ImGui::InputFloat("ZFAR", &p_cam->zFar);
		ShowVec3Editor("Up", p_cam->up);

		if (!p_cam->is_active && ImGui::Button("Make active")) {
			m_active_scene->MakeCameraActive(p_cam);
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


	void EditorLayer::ShowFileExplorer(std::string& path_name, std::string& t_entry_name, const std::vector<std::string>& valid_extensions, std::function<void()> valid_file_callback, std::function<void()> invalid_file_callback) {

		ImGui::SetNextWindowSize(ImVec2(file_explorer_window_size.x, file_explorer_window_size.y));
		if (ImGui::Begin("Files")) {

			ImGui::Text(path_name.c_str());
			if (path_name.find('/') != std::string::npos && ImGui::Button(ICON_FA_ARROW_LEFT "BACK")) {
				path_name = path_name.substr(0, path_name.find_last_of('/'));
			}

			if (ImGui::BeginTable("FILES", 5, 0, ImVec2(file_explorer_window_size.x, file_explorer_window_size.y), 100.f)) {
				for (const auto& entry : std::filesystem::directory_iterator(path_name)) {
					ImGui::TableNextColumn();

					const std::string entry_name = entry.path().filename().string();
					ImGui::PushID(entry_name.c_str());
					ImGui::GetFont()->Scale *= 2.f;
					ImGui::PushFont(ImGui::GetFont());

					//file/dir button
					if (ImGui::Button(entry.is_directory() ? ICON_FA_FOLDER : ICON_FA_FILE, ImVec2(125, 125))) {
						t_entry_name = entry_name;
						if (entry.is_directory()) {
							path_name += "/" + entry_name;
						}
						else {
							// entry is a file, check if extension valid
							std::string file_extension = entry_name.find('.') != std::string::npos ? entry_name.substr(entry_name.find_last_of('.')) : "";
							std::transform(file_extension.begin(), file_extension.end(), file_extension.begin(), ::toupper); // turn uppercase for comparison

							if (std::find(valid_extensions.begin(), valid_extensions.end(), file_extension) != valid_extensions.end()) {
								valid_file_callback(); // extension is valid
							}
							else {
								invalid_file_callback();
							}

						}
					};

					ImGui::PopID();
					ImGui::GetFont()->Scale *= 0.5f;
					ImGui::PopFont();

					ImGui::Text(entry_name.c_str());


				}
				ImGui::EndTable();
			}
		};
		ImGui::End();

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




	void EditorLayer::RenderMaterialTexture(const char* name, Texture2D*& p_tex) {
		ImGui::PushID(p_tex);
		if (p_tex) {
			ImGui::Text(std::format("{} texture - {}", name, p_tex->m_name).c_str());
			if (ImGui::ImageButton(ImTextureID(p_tex->GetTextureHandle()), ImVec2(75, 75))) {
				mp_selected_texture = p_tex;
				m_current_2d_tex_spec = p_tex->m_spec;
			};
		}
		else {
			ImGui::Text(std::format("{} texture - NONE", name).c_str());
			ImGui::ImageButton(ImTextureID(0), ImVec2(75, 75));
		}

		if (ImGui::IsItemHovered() && mp_dragged_texture) {
			p_tex = mp_dragged_texture;

			if (Window::IsMouseButtonDown(GLFW_MOUSE_BUTTON_2))
				p_tex = nullptr;
		}

		ImGui::PopID();
	}



	bool EditorLayer::H1TreeNode(const char* name) {
		float original_size = ImGui::GetFont()->Scale;
		ImGui::GetFont()->Scale *= 1.15f;
		ImGui::PushFont(ImGui::GetFont());
		bool r = ImGui::TreeNode(name);
		ImGui::GetFont()->Scale = original_size;
		ImGui::PopFont();
		return r;
	}

	bool EditorLayer::H2TreeNode(const char* name) {
		float original_size = ImGui::GetFont()->Scale;
		ImGui::GetFont()->Scale *= 1.1f;
		ImGui::PushFont(ImGui::GetFont());
		bool r = ImGui::TreeNode(name);
		ImGui::GetFont()->Scale = original_size;
		ImGui::PopFont();
		return r;
	}

}