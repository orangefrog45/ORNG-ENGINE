#include "pch/pch.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include "util/ImGuiLib.h"
#include "../extern/Icons.h"
#include "fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD.h"
#include "layers/EditorLayer.h"
#include "rendering/Renderer.h"
#include "scene/Scene.h"
#include "rendering/MeshInstanceGroup.h"
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
		m_active_scene = std::make_unique<Scene>();

		m_grid_mesh = std::make_unique<GridMesh>();
		m_grid_mesh->Init();
		mp_grid_shader = &Renderer::GetShaderLibrary().CreateShader("grid");
		mp_grid_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/GridVS.shader");
		mp_grid_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/GridFS.shader");
		mp_grid_shader->Init();

		mp_quad_shader = &Renderer::GetShaderLibrary().CreateShader("2d_quad");
		mp_quad_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/QuadVS.shader");
		mp_quad_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/QuadFS.shader");
		mp_quad_shader->Init();

		mp_picking_shader = &Renderer::GetShaderLibrary().CreateShader("picking");
		mp_picking_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/PickingVS.shader");
		mp_picking_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/PickingFS.shader");
		mp_picking_shader->Init();
		mp_picking_shader->AddUniform("comp_id");
		mp_picking_shader->AddUniform("world_transform");

		mp_highlight_shader = &Renderer::GetShaderLibrary().CreateShader("highlight");
		mp_highlight_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/HighlightVS.shader");
		mp_highlight_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/HighlightFS.shader");
		mp_highlight_shader->Init();
		mp_highlight_shader->AddUniform("transform");


		Texture2DSpec picking_spec;
		picking_spec.format = GL_RED_INTEGER;
		picking_spec.internal_format = GL_R32UI;
		picking_spec.storage_type = GL_UNSIGNED_INT;
		picking_spec.width = Window::GetWidth();
		picking_spec.height = Window::GetHeight();


		mp_picking_fb = &Renderer::GetFramebufferLibrary().CreateFramebuffer("picking");
		mp_picking_fb->AddRenderbuffer();
		mp_picking_fb->Add2DTexture("component_ids", GL_COLOR_ATTACHMENT0, picking_spec);


		SceneRenderer::SetActiveCamera(&m_editor_camera);
		SceneRenderer::SetActiveScene(&*m_active_scene);


		Texture2DSpec color_render_texture_spec;
		color_render_texture_spec.format = GL_RGB;
		color_render_texture_spec.internal_format = GL_RGB16F;
		color_render_texture_spec.storage_type = GL_FLOAT;
		color_render_texture_spec.width = Window::GetWidth();
		color_render_texture_spec.height = Window::GetHeight();


		mp_editor_pass_fb = &Renderer::GetFramebufferLibrary().CreateFramebuffer("editor_passes");
		mp_editor_pass_fb->AddShared2DTexture("shared_depth", Renderer::GetFramebufferLibrary().GetFramebuffer("gbuffer").GetTexture<Texture2D>("shared_depth"), GL_DEPTH_ATTACHMENT);
		mp_editor_pass_fb->AddShared2DTexture("shared_render_texture", Renderer::GetFramebufferLibrary().GetFramebuffer("post_processing").GetTexture<Texture2D>("shared_render_texture"), GL_COLOR_ATTACHMENT0);


		auto& entity = m_active_scene->CreateEntity("Orange");
		auto& entity2 = m_active_scene->CreateEntity("Cone");
		auto& entity3 = m_active_scene->CreateEntity("Other");
		auto mesh = entity.AddComponent<MeshComponent>("./res/meshes/oranges/orange.obj");
		auto mesh2 = entity2.AddComponent<MeshComponent>("./res/meshes/house-draft/source/house.fbx");

		auto mesh3 = entity3.AddComponent<MeshComponent>("./res/meshes/cube/cube.obj");
		auto script = entity.AddComponent<ScriptComponent>();
		entity.AddComponent<PointLightComponent>();
		entity.AddComponent<SpotLightComponent>();
		entity.AddComponent<ScriptComponent>();


		int width = 1;
		static auto fnSimplex = FastNoiseSIMD::NewFastNoiseSIMD(1);
		fnSimplex->SetNoiseType(FastNoiseSIMD::Perlin);
		fnSimplex->SetFractalType(FastNoiseSIMD::FBM);
		fnSimplex->SetFrequency(0.03f);

		for (int x = 0; x < width; x++) {
			for (int y = 0; y < width; y++) {
				for (int z = 0; z < width; z++) {
					auto& ent = m_active_scene->CreateEntity();
					auto m = ent.AddComponent<MeshComponent>("./res/meshes/cube/cube.obj");
					auto s = ent.AddComponent<ScriptComponent>();


					glm::mat4 rot = ExtraMath::Init3DRotateTransform(0, z * (360.f / width), 0);

					float x_pos = sinf(glm::radians((x + (y + z) / (720.f / width)) * (360.f / width))) * 200.f;
					float y_pos = cosf(glm::radians((x + (y + z) / (720.f / width)) * (360.f / width))) * 200.f;

					glm::vec3 new_pos = glm::vec3(rot * glm::vec4(x_pos, y_pos, z, 1));

					m->SetPosition(new_pos.x, new_pos.y, new_pos.z);
					m->SetScale(1.f, 1.f, 1.f);
				}
			}
		}

		/*for (int x = -width / 2.f; x < width / 2.f; x++) {
			for (int z = -width / 2.f; z < width / 2.f; z++) {
				constexpr int spacing = 500;
				auto& ent = m_active_scene->CreateEntity();
				auto m = ent.AddComponent<MeshComponent>("./res/meshes/cube/cube.obj");
				auto s = ent.AddComponent<ScriptComponent>();
				auto* pl = ent.AddComponent<PointLightComponent>();
				pl->max_distance = 100.f;
				pl->attenuation.constant = 0.05f;
				pl->attenuation.linear = 0.0f;
				pl->attenuation.exp = 0.0001f;
				pl->transform.SetPosition(x * spacing, 50.f, z * spacing);
				pl->color = glm::normalize(glm::vec3(rand() % 255, rand() % 255, rand() % 255));
				m->SetPosition(x * spacing, 50.f, z * spacing);


				s->OnUpdate = [pl, width] {
					static float x = 0;
					static float z = 0;
					x += 0.01f;
					z += 0.01f;
					if ((pl->transform.GetPosition().x < x && pl->transform.GetPosition().z < z)) {
						glm::vec3 old_color = pl->color;
						//pl->color = glm::normalize(glm::abs(old_color + glm::vec3(rand() % 255 - 255 * 0.5f, rand() % 255 - 255 * 0.5f, rand() % 255 - 255 * 0.5f) * 0.001f));
					}
					else {
						//pl->color = glm::normalize(glm::vec3(1));
					}

					if (x > width * spacing) {
						x = 0;
						z = 0;
					}
				};
			}
		}*/

		m_active_scene->LoadScene(&m_editor_camera);

		OAR_CORE_INFO("Editor layer initialized"); //add profiling func
	}


	void EditorLayer::Update() {

		if (!ImGui::GetIO().WantTextInput) {
			m_editor_camera.Update();
		}


		m_active_scene->Update();
	}




	void EditorLayer::RenderUI() {

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();
		ImGui::Begin("Tools", nullptr);
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Text(std::format("Draw calls: {}", Renderer::Get().m_draw_call_amount).c_str());
		Renderer::ResetDrawCallCounter();

		DisplayEntityEditor();
		RenderSceneGraph();

		ImGui::End();

		ShowAssetManager();

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

		settings.display_depth_map = m_display_settings.depth_map_view;

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




	void EditorLayer::DoPickingPass() {

		mp_picking_fb->Bind();
		mp_picking_shader->ActivateProgram();

		GL_StateManager::ClearDepthBits();
		GL_StateManager::ClearBitsUnsignedInt();

		for (auto& mesh : m_active_scene->m_mesh_components) {
			if (!mesh || !mesh->GetMeshData()) continue;

			if (mesh->GetMeshData()->GetLoadStatus() == true) {
				mp_picking_shader->SetUniform<unsigned int>("comp_id", mesh->GetEntityHandle());
				mp_picking_shader->SetUniform("world_transform", mesh->GetWorldTransform()->GetMatrix());

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

		auto* meshc = m_active_scene->GetEntity(m_selected_entity_id)->GetComponent<MeshComponent>();

		if (!meshc)
			return;

		mp_editor_pass_fb->Bind();
		mp_highlight_shader->ActivateProgram();
		mp_highlight_shader->SetUniform("transform", meshc->GetWorldTransform()->GetMatrix());

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
			ImGuiLib::ShowColorVec3Editor("Color", m_active_scene->m_global_fog.color);
			ImGui::TreePop();
		}
	}




	void EditorLayer::RenderGrid() {
		GL_StateManager::BindSSBO(m_grid_mesh->m_ssbo_handle, GL_StateManager::SSBO_BindingPoints::TRANSFORMS);
		m_grid_mesh->CheckBoundary(m_editor_camera.GetPos());
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
				m_active_scene->m_terrain.ResetTerrainQuadtree(m_editor_camera);

			ImGui::TreePop();
		}
	}


	void EditorLayer::ShowAssetManager() {

		constexpr int asset_window_width = 1600;
		constexpr int asset_window_height = 400;
		constexpr int num_table_columns = 4;
		constexpr int child_section_width = asset_window_width * 0.95;
		constexpr int child_section_height = asset_window_height * 0.95;
		constexpr int table_outer_size = 200;
		constexpr ImVec4 edit_button_color = ImVec4(0, 0.2, 0.8, 0.5);
		constexpr float dummy_spacing_size_vertical = 20.f;

		static std::string error_message = "";
		static std::string path = "./res/meshes";
		static std::string entry_name = "";


		ImGui::SetNextWindowSize(ImVec2(asset_window_width, asset_window_height));
		ImGui::Begin("Assets");
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

				ImGuiLib::ShowFileExplorer(path, entry_name, mesh_file_extensions, success_callback, error_callback);

				ImGui::TreePop();
			} // END MESH FILE EXPLORER


			if (ImGui::BeginTable("Meshes", num_table_columns, ImGuiTableFlags_Borders)) // MESH VIEWING TABLE
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


		static Texture2D* p_selected_texture = nullptr;
		static Texture2DSpec current_spec;

		if (p_selected_texture) { // TEXTURE EDITOR
			bool window_visible_flag = true;
			ImGui::Begin("Texture Editor");

			if (ImGui::Button("X"))
				p_selected_texture = nullptr;

			// Check validity again as button could've changed it
			if (p_selected_texture)
				RenderTextureEditor(p_selected_texture, current_spec);


			ImGui::End();
		}

		if (ImGui::BeginTabItem("Textures")) // TEXTURE TAB
		{
			if (ImGui::TreeNode("Add texture")) { // TEXTURE FILE EXPLORER

				ImGui::TextColored(ImVec4(1, 0, 0, 1), error_message.c_str());

				static std::string file_extension = "";
				static std::vector<std::string> valid_filepaths = { ".PNG", ".JPG", ".JPEG" };

				//setting up file explorer callbacks
				static std::function<void()> success_callback = [this] {
					m_active_scene->CreateTexture2DAsset(path + "/" + entry_name);
					error_message.clear();
				};

				static std::function<void()> fail_callback = [] {
					error_message = file_extension.empty() ? "" : "Invalid file type: " + file_extension; // if file extension is empty it's a directory, so no error
				};

				ImGuiLib::ShowFileExplorer(path, entry_name, valid_filepaths, success_callback, fail_callback);

				ImGui::TreePop();
			} // END TEXTURE FILE EXPLORER


			if (ImGui::TreeNode("Texture viewer")) // TEXTURE VIEWING TREE NODE
			{

				if (ImGui::BeginChild("##Texture viewing section", ImVec2(child_section_width, child_section_height))) {
					// Create table for textures 
					if (ImGui::BeginTable("Textures", num_table_columns, ImGuiTableFlags_Borders | ImGuiTableFlags_PadOuterX)); // TEXTURE VIEWING TABLE
					{
						// Push textures into table 
						for (auto& p_texture : m_active_scene->m_texture_2d_assets)
						{
							ImGui::PushID(p_texture);
							ImGui::TableNextColumn();


							if (ImGui::ImageButton(ImTextureID(p_texture->GetTextureHandle()), ImVec2(table_outer_size, table_outer_size))) {
								p_selected_texture = p_texture;
								current_spec = p_selected_texture->m_spec;
							};

							ImGui::Text(p_texture->m_spec.filepath.substr(p_texture->m_spec.filepath.find_last_of('/') + 1).c_str());

							ImGui::PopID();

						}

						ImGui::EndTable();
					} // END TEXTURE VIEWING TABLE
				};
				ImGui::EndChild();
				ImGui::TreePop();
			}
			ImGui::EndTabItem();
		} // END TEXTURE TAB



		static Material* p_selected_material = nullptr;

		if (ImGui::BeginTabItem("Materials")) { // MATERIAL TAB

			if (p_selected_material && ImGui::Begin("Material editor")) {

				bool is_window_displayed = true;

				if (ImGui::SmallButton("X"))
					is_window_displayed = false;

				ImGui::Text("Name: ");
				ImGui::SameLine();
				ImGui::InputText("##name input", &p_selected_material->name);
				ImGui::Spacing();

				ImGui::Text(std::format("Diffuse Texture - {}", p_selected_material->diffuse_texture->m_name).c_str());
				if (ImGui::ImageButton(ImTextureID(p_selected_material->diffuse_texture->GetTextureHandle()), ImVec2(100, 100))) {
					p_selected_texture = p_selected_material->diffuse_texture;
					current_spec = p_selected_material->diffuse_texture->m_spec;
				};

				if (p_selected_material->normal_map_texture) {
					ImGui::TableNextColumn();
					ImGui::Text(std::format("Normal Texture - {}", p_selected_material->normal_map_texture->m_name).c_str());
					if (ImGui::ImageButton(ImTextureID(p_selected_material->normal_map_texture->GetTextureHandle()), ImVec2(100, 100))) {
						p_selected_texture = p_selected_material->normal_map_texture;
						current_spec = p_selected_material->normal_map_texture->m_spec;

					};
					ImGui::TableNextColumn();
				}

				if (p_selected_material->specular_texture) {
					ImGui::TableNextColumn();
					ImGui::Text(std::format("Specular Texture - {}", p_selected_material->specular_texture->m_name).c_str());
					if (ImGui::ImageButton(ImTextureID(p_selected_material->specular_texture->GetTextureHandle()), ImVec2(100, 100))) {
						p_selected_texture = p_selected_material->specular_texture;
						current_spec = p_selected_material->specular_texture->m_spec;

					};
				}


				ImGui::Text("Colors");
				ImGui::Spacing();
				ImGuiLib::ShowVec3Editor("Ambient", p_selected_material->ambient_color);
				ImGuiLib::ShowVec3Editor("Diffuse", p_selected_material->diffuse_color);
				ImGuiLib::ShowVec3Editor("Specular", p_selected_material->specular_color);

				if (!is_window_displayed)
					p_selected_material = nullptr;


				ImGui::End();

			}


			if (ImGui::BeginChild("Material section", ImVec2(child_section_width, child_section_height))) {
				if (ImGui::BeginTable("Material viewer", num_table_columns, ImGuiTableFlags_Borders | ImGuiTableFlags_PadOuterX, ImVec2(child_section_width, child_section_height))) { //MATERIAL VIEWING TABLE

					for (auto* p_material : m_active_scene->m_materials) {
						ImGui::TableNextColumn();

						ImGui::PushID(p_material);

						if (p_material->diffuse_texture) {
							if (ImGui::ImageButton(ImTextureID(p_material->diffuse_texture->m_texture_obj), ImVec2(table_outer_size, table_outer_size)))
								p_selected_material = p_material;
						}

						ImGui::Text(p_material->name.c_str());

						ImGui::PopID();
					}

					Renderer::GetShaderLibrary().UpdateMaterialUBO(m_active_scene->m_materials);
					ImGui::EndTable();
				} //END MATERIAL VIEWING TABLE
				ImGui::EndChild();
			}
			ImGui::EndTabItem();
		} //END MATERIAL TAB

		ImGui::GetStyle().CellPadding = ImVec2(4.f, 4.f);
		ImGui::EndTabBar();
		ImGui::End();
	}


	void EditorLayer::RenderTextureEditor(Texture2D* selected_texture, Texture2DSpec& spec) {

		ImGui::Text(selected_texture->GetSpec().filepath.c_str());

		const char* wrap_modes[] = { "REPEAT", "CLAMP TO EDGE" };
		const char* internal_formats[] = { "SRGB8", "RGB8", "RGBA8", "SRGB8_A8" };
		const char* formats[] = { "RGB", "RGBA" };

		static int selected_wrap_mode = spec.wrap_params == GL_LINEAR ? 0 : 1;
		static int selected_format = 0;
		static int selected_internal_format = 0;

		ImGui::Text("Wrap mode");
		ImGui::SameLine();
		ImGui::Combo("##Wrap mode", &selected_wrap_mode, wrap_modes, IM_ARRAYSIZE(wrap_modes));
		spec.wrap_params = selected_wrap_mode == 0 ? GL_REPEAT : GL_CLAMP_TO_EDGE;

		ImGui::Text("Format");
		ImGui::SameLine();
		ImGui::Combo("##Format", &selected_format, formats, IM_ARRAYSIZE(formats));
		spec.format = selected_format == 0 ? GL_RGB : GL_RGBA;

		ImGui::Text("Internal format");
		ImGui::SameLine();
		ImGui::Combo("##Internal format", &selected_internal_format, internal_formats, IM_ARRAYSIZE(internal_formats));
		switch (selected_internal_format) {
		case 0:
			spec.internal_format = GL_SRGB8;
		case 1:
			spec.internal_format = GL_RGB8;
		case 2:
			spec.internal_format = GL_RGBA8;
		case 3:
			spec.internal_format = GL_SRGB8_ALPHA8;
		}

		if (ImGui::Button("Load")) {
			selected_texture->SetSpec(spec, false);
			selected_texture->LoadFromFile();
		}
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

		if (ImGui::CollapsingHeader("Entity editor")) {
			std::string ent_text = std::format("Entity '{}'", entity->name);
			ImGui::Text(ent_text.c_str());

			if (meshc) {

				if (ImGui::TreeNode("Mesh")) {
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


			if (plight) {
				if (ImGui::TreeNode("Pointlight")) {
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

			if (slight) {
				if (ImGui::TreeNode("Spotlight")) {
					ImGui::PushID(slight);
					ImGui::SameLine();
					if (ImGui::Button("X", ImVec2(25, 25))) {
						entity->DeleteComponent<SpotLightComponent>();
					};
					RenderSpotlightEditor(slight);
					ImGui::TreePop();
					ImGui::PopID();
				}
			}
			else {
				ImGui::Text("Spotlight");
				ImGui::SameLine();
				if (ImGui::Button("+", ImVec2(25, 25))) {
					entity->AddComponent<SpotLightComponent>();
				}
			}
		}
	}



	// CONTROL WINDOWS ------------------------------------------------------------------------


	void EditorLayer::RenderMeshComponentEditor(MeshComponent* comp) {
		static MeshComponentData mesh_data;

		ImGui::PushID(comp);
		//static unsigned int old_material_id = comp->m_material_id;
		//mesh_data.material_id = comp->m_material_id;

		ImGui::Text("Material ID");
		ImGui::SameLine();
		ImGui::InputInt("##material id", &mesh_data.material_id);


		//if (mesh_data.material_id < 0 || mesh_data.material_id >= m_active_scene->m_materials.size() - 1) // Don't allow material id (index) to go out of array bounds
			//mesh_data.material_id = old_material_id;

		//if (old_material_id != mesh_data.material_id) {
			//comp->SetMaterialID(mesh_data.material_id);
			//old_material_id = mesh_data.material_id;
		//}


		ImGuiIO& io = ImGui::GetIO();
		ImGuizmo::BeginFrame();
		ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

		static ImGuizmo::OPERATION current_operation = ImGuizmo::TRANSLATE;
		static ImGuizmo::MODE current_mode = ImGuizmo::WORLD;

		ImGui::Dummy(ImVec2(0.f, 20.f));
		ImGui::Text("Transform");

		if (ImGui::RadioButton("World", current_mode == ImGuizmo::WORLD))
			current_mode = ImGuizmo::WORLD;
		ImGui::SameLine();
		if (ImGui::RadioButton("Local", current_mode == ImGuizmo::LOCAL))
			current_mode = ImGuizmo::LOCAL;

		if (Window::IsKeyDown(GLFW_KEY_1))
			current_operation = ImGuizmo::TRANSLATE;
		else if (Window::IsKeyDown(GLFW_KEY_2))
			current_operation = ImGuizmo::SCALE;
		else if (Window::IsKeyDown(GLFW_KEY_3))
			current_operation = ImGuizmo::ROTATE;

		glm::mat4 current_operation_matrix = glm::mat4(1);

		current_operation_matrix = comp->GetWorldTransform()->GetMatrix();

		glm::vec3 matrix_translation = comp->mp_transform->m_pos;
		glm::vec3 matrix_rotation = comp->mp_transform->m_rotation;
		glm::vec3 matrix_scale = comp->mp_transform->m_scale;


		if (ImGuiLib::ShowVec3Editor("Tr", matrix_translation))
			comp->SetPosition(matrix_translation);

		if (ImGuiLib::ShowVec3Editor("Rt", matrix_rotation))
			comp->SetOrientation(matrix_rotation);

		if (ImGuiLib::ShowVec3Editor("Sc", matrix_scale))
			comp->SetScale(matrix_scale);

		glm::mat4 delta_matrix = glm::mat4(1);
		if (ImGuizmo::Manipulate(&m_editor_camera.GetViewMatrix()[0][0], &m_editor_camera.GetProjectionMatrix()[0][0], current_operation, current_mode, &current_operation_matrix[0][0], &delta_matrix[0][0], nullptr) && ImGuizmo::IsUsing()) {

			switch (current_operation) {
			case ImGuizmo::TRANSLATE:
				ImGuizmo::DecomposeMatrixToComponents(&current_operation_matrix[0][0], &matrix_translation[0], &matrix_rotation[0], &matrix_scale[0]);
				comp->SetPosition(matrix_translation);
				break;
			case ImGuizmo::SCALE:
				ImGuizmo::DecomposeMatrixToComponents(&current_operation_matrix[0][0], &matrix_translation[0], &matrix_rotation[0], &matrix_scale[0]);
				comp->SetScale(matrix_scale);
				break;
			case ImGuizmo::ROTATE: // Have to use rotation delta to prevent orientation bugs
				ImGuizmo::DecomposeMatrixToComponents(&current_operation_matrix[0][0], &matrix_translation[0], &matrix_rotation[0], &matrix_scale[0]);
				comp->SetOrientation(matrix_rotation);
				break;
			}
		};

		ImGui::Dummy(ImVec2(0.f, 20.f));
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

		ImGuiLib::ShowVec3Editor("Direction", light_data.direction);

		ImGuiLib::ShowColorVec3Editor("Color", light_data.base.color);
		ImGuiLib::ShowVec3Editor("Position", light_data.base.pos);

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
			ImGuiLib::ShowColorVec3Editor("Color", light_data.light_color);

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
		ImGuiLib::ShowColorVec3Editor("Color", light_data.color);
		ImGuiLib::ShowVec3Editor("Position", light_data.pos);

		light->attenuation.constant = light_data.atten_constant;
		light->attenuation.linear = light_data.atten_linear;
		light->attenuation.exp = light_data.atten_exp;
		light->max_distance = light_data.max_distance;
		light->color = light_data.color;
		light->transform.SetPosition(light_data.pos);
	}


}