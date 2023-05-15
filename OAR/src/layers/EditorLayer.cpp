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
#include "core/Input.h"
#include "rendering/Renderpasses.h"

namespace ORNG {

	void EditorLayer::Init() {
		m_active_scene = std::make_unique<Scene>();
		ImGuiLib::Init();

		RenderPasses::InitPasses();
		Renderer::SetActiveScene(*m_active_scene);
		Renderer::SetActiveCamera(&m_editor_camera);
		m_active_scene->Init();
		auto& entity = m_active_scene->CreateEntity("Orange");
		auto& entity2 = m_active_scene->CreateEntity("Cone");
		auto& entity3 = m_active_scene->CreateEntity("Other");
		auto mesh = entity.AddComponent<MeshComponent>("./res/meshes/oranges/orange.obj");
		auto mesh2 = entity2.AddComponent<MeshComponent>("./res/meshes/StoneArchLarge_Fbx.fbx");
		auto mesh3 = entity3.AddComponent<MeshComponent>("./res/meshes/cube/cube.obj");
		auto script = entity.AddComponent<ScriptComponent>();
		entity.AddComponent<PointLightComponent>();
		entity.AddComponent<SpotLightComponent>();
		entity.AddComponent<ScriptComponent>();

		entity.GetComponent<ScriptComponent>()->OnUpdate = [mesh] {
			glm::vec3 old_rot = mesh->GetWorldTransform()->GetRotation();
			mesh->SetOrientation(old_rot.x, old_rot.y + 0.00025f * Input::GetTimeStep(), old_rot.z);
		};


		int width = 3;
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

					s->OnUpdate = [s, x, y, z, this, width] {
						auto meshc = s->GetEntity()->GetComponent<MeshComponent>();

						static float* f = fnSimplex->GetCubicSet(0, 0, 0, width, width, width);

						static NoiseSettings old_settings = m_noise_settings;

						if (old_settings.cutoff != m_noise_settings.cutoff || old_settings.frequency != m_noise_settings.frequency || old_settings.seed != m_noise_settings.seed || old_settings.type != m_noise_settings.type) {
							old_settings = m_noise_settings;

							fnSimplex->SetNoiseType(m_noise_settings.type);
							fnSimplex->SetSeed(m_noise_settings.seed);
							fnSimplex->SetFrequency(m_noise_settings.frequency);
							switch (m_noise_settings.type) {
							case FastNoiseSIMD::Cellular:
								f = fnSimplex->GetCellularSet(0, 0, 0, width, width, width);
								break;
							case FastNoiseSIMD::Cubic:
								f = fnSimplex->GetCubicSet(0, 0, 0, width, width, width);
								break;
							case FastNoiseSIMD::Perlin:
								f = fnSimplex->GetPerlinSet(0, 0, 0, width, width, width);
								break;
							case FastNoiseSIMD::Simplex:
								f = fnSimplex->GetSimplexSet(0, 0, 0, width, width, width);
								break;
							}

						}

						glm::vec3 old_pos = meshc->GetWorldTransform()->GetPosition();
						glm::vec3 pos_to_world_vec = glm::normalize(RenderPasses::current_pos - old_pos);
						glm::vec3 offset_pos = { x * 0.095f, y * 0.095f, z * 0.095f };

						if (f[z * width * width + y * width + x] > m_noise_settings.cutoff) {
							meshc->SetPosition(old_pos + (offset_pos + pos_to_world_vec) * static_cast<float>(Input::GetTimeStep()) * 0.0001f);
						}
						else {
							if (glm::length(old_pos - pos_to_world_vec) >= 300.f)
								return;

							meshc->SetPosition(old_pos - (offset_pos + pos_to_world_vec) * static_cast<float>(Input::GetTimeStep()) * 0.0001f);
						}

					};

					glm::mat4 rot = ExtraMath::Init3DRotateTransform(0, z * 7.2f, 0);

					float x_pos = sinf(glm::radians((x + (y + z) / 18.f) * 7.2f)) * 200.f;
					float y_pos = cosf(glm::radians((x + (y + z) / 18.f) * 7.2f)) * 200.f;

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
		if (Input::IsKeyDown('U')) { // quick teleport, add as scriptable component once camera components implemented
			glm::vec3 pos = RenderPasses::current_pos;
			m_editor_camera.SetPosition(pos.x, pos.y + 30.f, pos.z);
		}

		if (!ImGui::GetIO().WantTextInput) {
			m_editor_camera.Update();
		}


		m_active_scene->m_terrain.UpdateTerrainQuadtree();
		m_active_scene->UpdateScene();
	}

	void EditorLayer::ShowUIWindow() {
		CreateBaseWindow();

		DisplayEntityEditor();
		DisplayFogHeader();

		ImGui::Checkbox("Show depth map", &RenderPasses::depth_map_view_active);

		if (ImGui::TreeNode("noise")) {
			ImGui::SliderFloat("frequency", &m_noise_settings.frequency, 0.f, 1.f);
			ImGui::SliderFloat("cutoff point", &m_noise_settings.cutoff, -1.f, 1.f);
			ImGui::InputInt("seed", &m_noise_settings.seed);

			static int selected_mode = 0;
			const char* modes[] = { "CELLULAR", "SIMPLEX", "CUBIC", "PERLIN" };

			ImGui::Combo("type", &selected_mode, modes, IM_ARRAYSIZE(modes));
			switch (selected_mode) {
			case 0:
				m_noise_settings.type = FastNoiseSIMD::Cellular;
				break;
			case 1:
				m_noise_settings.type = FastNoiseSIMD::Simplex;
				break;
			case 2:
				m_noise_settings.type = FastNoiseSIMD::Cubic;
				break;
			case 3:
				m_noise_settings.type = FastNoiseSIMD::Perlin;
				break;
			}

			ImGui::TreePop();
		}

		DisplaySceneEntities();
		//DisplaySceneData();
		//DisplayDebugControls(Renderer::m_framebuffer_library.dir_depth_fb.GetDepthMap());
		EnableDirectionalLightControls();

		ImGui::End();

		ShowAssetManager();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	void EditorLayer::ShowDisplayWindow() {

		glm::mat4 cam_mat = m_editor_camera.GetViewMatrix();
		glm::mat4 proj_mat = m_editor_camera.GetProjectionMatrix();
		Renderer::GetShaderLibrary().SetMatrixUBOs(proj_mat, cam_mat);
		Renderer::GetShaderLibrary().SetGlobalLighting(m_active_scene->m_directional_light, m_active_scene->m_global_ambient_lighting);
		RenderPasses::DoGBufferPass();

		if (Input::IsMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT) && !ImGui::GetIO().WantCaptureMouse) {
			RenderPasses::DoPickingPass();
			RenderPasses::gbuffer_positions_sample_flag = true;
		}
		RenderPasses::DoDepthPass();
		RenderPasses::DoLightingPass();
		RenderPasses::DoFogPass();
		RenderPasses::DoDrawToQuadPass();

		OAR_CORE_INFO("DRAW CALLS: {0}", Renderer::Get().m_draw_call_amount);
		Renderer::Get().m_draw_call_amount = 0;
	}

	void EditorLayer::DisplayFogHeader() {
		if (ImGui::CollapsingHeader("Fog")) {
			ImGui::Text("Intensity");
			ImGui::SameLine();
			ImGui::SliderFloat("##intensity", &RenderPasses::fog_data.intensity, 0.f, 0.25f);
			ImGui::Text("Hardness");
			ImGui::SameLine();
			ImGui::SliderFloat("##hardness", &RenderPasses::fog_data.hardness, 0.f, 0.25f);
			ImGuiLib::ShowColorVec3Editor("Color", RenderPasses::fog_data.color);
		}
	}

	void EditorLayer::ShowAssetManager() {
		ImGui::SetNextWindowSize(ImVec2(1500, 400));
		ImGui::Begin("Assets");
		ImGui::AlignTextToFramePadding();
		ImGui::BeginTabBar("Selection");

		static std::string error_message = "";
		static bool click_flag = false;
		static std::string path = "./res/meshes";
		static std::string entry_name = "";

		static Texture2D* selected_texture = nullptr;
		static Texture2DSpec current_spec;


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
					{
						ImGui::TextColored(ImVec4(0, 1, 0, 1), "LOADED");
					}
					else
					{
						ImGui::TextColored(ImVec4(1, 0, 0, 1), "NOT LOADED");
					}
					ImGui::PopID();
				}

				ImGui::EndTable();
				ImGui::EndTabItem();

			} // END MESH VIEWING TABLE
		} //	 END MESH TAB


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


			if (selected_texture) { // TEXTURE EDITOR
				ImGuiLib::ShowTextureEditor(selected_texture, current_spec);
			} // END TEXTURE EDITOR


			if (ImGui::TreeNode("Texture viewer")) // TEXTURE VIEWING TREE NODE
			{
				ImGui::GetStyle().CellPadding = ImVec2(11.f, 15.f);

				if (ImGui::BeginChild("##Texture viewing section", ImVec2(1300, 350))) {
					// Create table for textures 
					if (ImGui::BeginTable("Textures", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_PadOuterX, ImVec2(1200, 300), 400.f)); // TEXTURE VIEWING TABLE
					{
						// Push textures into table 
						for (auto& data : m_active_scene->m_texture_2d_assets)
						{
							ImGui::PushID(data);
							ImGui::TableNextColumn();

							ImGui::Text(data->m_spec.filepath.substr(data->m_spec.filepath.find_last_of('/') + 1).c_str());
							ImGui::SameLine();
							ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0.2, 0.8, 0.5));
							if (ImGui::SmallButton("EDIT"))
							{
								selected_texture = data;
								current_spec = selected_texture->m_spec;
							}
							ImGui::PopStyleColor();


							if (data->m_texture_obj != 0)
							{
								ImGui::TextColored(ImVec4(0, 1, 0, 1), "LOADED");
							}
							else
							{
								ImGui::TextColored(ImVec4(1, 0, 0, 1), "NOT LOADED");
							}
							ImGui::Image(ImTextureID(data->GetTextureHandle()), ImVec2(250, 250));
							ImGui::PopID();
						}

						ImGui::GetStyle().CellPadding = ImVec2(4.f, 4.f);
						ImGui::EndTable();
					} // END TEXTURE VIEWING TABLE
					ImGui::EndChild();
				};
				ImGui::TreePop();
			} // END TEXTURE VIEWING TREE NODE
			ImGui::EndTabItem();
		} // END TEXTURE TAB

		if (ImGui::BeginTabItem("Materials")) { // MATERIAL TAB
			if (ImGui::BeginChild("Material section", ImVec2(1100, 300))) {
				if (ImGui::BeginTable("Material viewer", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_PadOuterX, ImVec2(1100, 300), 250.f)) { //MATERIAL VIEWING TABLE
					ImGui::GetStyle().CellPadding = ImVec2(11.f, 15.f);

					for (auto* material : m_active_scene->m_materials) {
						ImGui::TableNextColumn();

						ImGui::PushID(material);
						if (material->diffuse_texture) {
							ImGui::Image(ImTextureID(material->diffuse_texture->m_texture_obj), ImVec2(100, 100));
						}

						ImGui::Text("Ambient color");
						ImGui::Text(std::to_string(material->ambient_color.x).c_str());
						ImGui::SameLine();
						ImGui::Text(std::to_string(material->ambient_color.y).c_str());
						ImGui::SameLine();
						ImGui::Text(std::to_string(material->ambient_color.z).c_str());



						ImGui::Text("Diffuse color");
						ImGui::Text(std::to_string(material->diffuse_color.x).c_str());
						ImGui::SameLine();
						ImGui::Text(std::to_string(material->diffuse_color.y).c_str());
						ImGui::SameLine();
						ImGui::Text(std::to_string(material->diffuse_color.z).c_str());

						ImGui::Text("Specular color");
						ImGui::Text(std::to_string(material->specular_color.x).c_str());
						ImGui::SameLine();
						ImGui::Text(std::to_string(material->specular_color.y).c_str());
						ImGui::SameLine();
						ImGui::Text(std::to_string(material->specular_color.z).c_str());
						ImGui::PopID();
					}

					ImGui::EndTable();
				} //END MATERIAL VIEWING TABLE
				ImGui::EndChild();
			}
			ImGui::EndTabItem();
		} //END MATERIAL TAB

		ImGui::EndTabBar();
		ImGui::End();
	}


	void EditorLayer::CreateBaseWindow() {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("Tools", nullptr);
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	}


	void EditorLayer::DisplaySceneEntities() {
		if (ImGui::CollapsingHeader("Scene")) {
			for (auto& entity : m_active_scene->m_entities) {
				if (entity && ImGui::Button(entity->name, ImVec2(200, 25))) {
					RenderPasses::current_entity_id = entity->GetID();
				}
			}
		}
	}


	void EditorLayer::DisplayEntityEditor() {
		auto entity = m_active_scene->GetEntity(RenderPasses::current_entity_id);
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

						for (auto& mesh_asset : m_active_scene->m_mesh_assets) {
							std::string mesh_filename = mesh_asset->GetFilename().substr(mesh_asset->GetFilename().find_last_of('/') + 1);
							std::transform(mesh_filename.begin(), mesh_filename.end(), mesh_filename.begin(), ::toupper);
							std::transform(input_filename.begin(), input_filename.end(), input_filename.begin(), ::toupper);

							if (mesh_filename == input_filename) {
								m_active_scene->SortMeshIntoInstanceGroup(meshc, mesh_asset);
								meshc = entity->GetComponent<MeshComponent>();
							}

						}
					};

					if (meshc->mp_mesh_asset) {
						// highlight the currently selected mesh component
						Shader& highlight_shader = Renderer::GetShaderLibrary().GetShader("highlight");
						highlight_shader.ActivateProgram();
						highlight_shader.SetUniform("transform", meshc->GetWorldTransform()->GetMatrix());
						Renderer::DrawMesh(meshc->GetMeshData(), false);

						EnableMeshComponentControls(meshc);
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
					EnablePointLightControls(plight);
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
					EnableSpotLightControls(slight);
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
		}
	}



	// CONTROL WINDOWS ------------------------------------------------------------------------


	void EditorLayer::EnableMeshComponentControls(MeshComponent* comp) {
		ImGui::PushID(comp);
		static unsigned int old_material_id = comp->m_material_id;
		m_selected_mesh_data.scale = comp->GetWorldTransform()->GetScale();
		m_selected_mesh_data.position = comp->GetWorldTransform()->GetPosition();
		m_selected_mesh_data.rotation = comp->GetWorldTransform()->GetRotation();
		m_selected_mesh_data.material_id = comp->m_material_id;

		ImGui::Text("Material ID");
		ImGui::SameLine();
		ImGui::InputInt("##material id", &m_selected_mesh_data.material_id);

		ImGuiLib::ShowVec3Editor("Position", m_selected_mesh_data.position);
		ImGuiLib::ShowVec3Editor("Rotation", m_selected_mesh_data.rotation);
		ImGuiLib::ShowVec3Editor("Scale", m_selected_mesh_data.scale);

		if (m_selected_mesh_data.material_id < 0) // Don't allow material id (index) to go below 0
			m_selected_mesh_data.material_id = 0;

		if (old_material_id != m_selected_mesh_data.material_id) {
			comp->SetMaterialID(m_selected_mesh_data.material_id);
			old_material_id = m_selected_mesh_data.material_id;
		}
		comp->SetPosition(m_selected_mesh_data.position);
		comp->SetScale(m_selected_mesh_data.scale);
		comp->SetOrientation(m_selected_mesh_data.rotation);
		ImGui::PopID();
	};

	void EditorLayer::EnableSpotLightControls(SpotLightComponent* light) {

		m_spotlight_config_data.aperture = glm::degrees(acosf(light->aperture));
		m_spotlight_config_data.direction = light->m_light_direction_vec;
		m_spotlight_config_data.base.atten_constant = light->attenuation.constant;
		m_spotlight_config_data.base.atten_exp = light->attenuation.exp;
		m_spotlight_config_data.base.atten_linear = light->attenuation.linear;
		m_spotlight_config_data.base.color = light->color;
		m_spotlight_config_data.base.max_distance = light->max_distance;
		m_spotlight_config_data.base.pos = light->transform.GetPosition();

		ImGui::PushItemWidth(200.f);
		ImGui::SliderFloat("constant", &m_spotlight_config_data.base.atten_constant, 0.0f, 1.0f);
		ImGui::SliderFloat("linear", &m_spotlight_config_data.base.atten_linear, 0.0f, 1.0f);
		ImGui::SliderFloat("exp", &m_spotlight_config_data.base.atten_exp, 0.0f, 0.1f);
		ImGui::SliderFloat("max distance", &m_spotlight_config_data.base.max_distance, 0.0f, 200.0f);

		ImGui::Text("Aperture");
		ImGui::SameLine();
		ImGui::SliderFloat("##a", &m_spotlight_config_data.aperture, 0.f, 90.f);

		ImGui::PopItemWidth();

		ImGuiLib::ShowVec3Editor("Direction", m_spotlight_config_data.direction);

		ImGuiLib::ShowColorVec3Editor("Color", m_spotlight_config_data.base.color);
		ImGuiLib::ShowVec3Editor("Position", m_spotlight_config_data.base.pos);

		light->attenuation.constant = m_spotlight_config_data.base.atten_constant;
		light->attenuation.linear = m_spotlight_config_data.base.atten_linear;
		light->attenuation.exp = m_spotlight_config_data.base.atten_exp;
		light->max_distance = m_spotlight_config_data.base.max_distance;
		light->color = m_spotlight_config_data.base.color;
		light->transform.SetPosition(m_spotlight_config_data.base.pos);
		light->SetLightDirection(m_spotlight_config_data.direction.x, m_spotlight_config_data.direction.y, m_spotlight_config_data.direction.z);
		light->SetAperture(m_spotlight_config_data.aperture);
	}

	void EditorLayer::EnableDebugControls(unsigned int depth_map_texture) {
		ImGui::Text("DEBUG CONTROLS");
		ImGui::Checkbox("Toggle depth view", &m_debug_config_data.depth_map_view);

		if (m_debug_config_data.depth_map_view == true) {
			ImGui::End();
			ImGui::Begin("DEPTH MAP");
			ImGui::Image((void*)depth_map_texture, ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
			ImGui::End();
			ImGui::Begin("Tools");
		}
	}

	void EditorLayer::EnableDirectionalLightControls() {

		if (ImGui::CollapsingHeader("Directional light")) {
			ImGui::Text("DIR LIGHT CONTROLS");
			ImGui::PushItemWidth(200.f);
			ImGui::SliderFloat("x-pos", &m_dir_light_data.light_position.x, -1.0f, 1.0f);
			ImGui::SliderFloat("y-pos", &m_dir_light_data.light_position.y, -1.0f, 1.0f);
			ImGui::SliderFloat("z-pos", &m_dir_light_data.light_position.z, -1.0f, 1.0f);
			ImGuiLib::ShowColorVec3Editor("Color", m_dir_light_data.light_color);
			ImGui::PopItemWidth();

			m_active_scene->m_directional_light.color = glm::vec3(m_dir_light_data.light_color.x, m_dir_light_data.light_color.y, m_dir_light_data.light_color.z);
			m_active_scene->m_directional_light.SetLightDirection(m_dir_light_data.light_position);
		}
	}


	void EditorLayer::EnablePointLightControls(PointLightComponent* light) {

		m_pointlight_config_data.color = light->color;
		m_pointlight_config_data.atten_exp = light->attenuation.exp;
		m_pointlight_config_data.atten_linear = light->attenuation.linear;
		m_pointlight_config_data.atten_constant = light->attenuation.constant;
		m_pointlight_config_data.max_distance = light->max_distance;
		m_pointlight_config_data.pos = light->transform.GetPosition();

		ImGui::PushItemWidth(200.f);
		ImGui::SliderFloat("constant", &m_pointlight_config_data.atten_constant, 0.0f, 1.0f);
		ImGui::SliderFloat("linear", &m_pointlight_config_data.atten_linear, 0.0f, 1.0f);
		ImGui::SliderFloat("exp", &m_pointlight_config_data.atten_exp, 0.0f, 0.1f);
		ImGui::SliderFloat("max distance", &m_pointlight_config_data.max_distance, 0.0f, 5000.0f);
		ImGui::PopItemWidth();
		ImGuiLib::ShowColorVec3Editor("Color", m_pointlight_config_data.color);
		ImGuiLib::ShowVec3Editor("Position", m_pointlight_config_data.pos);

		light->attenuation.constant = m_pointlight_config_data.atten_constant;
		light->attenuation.linear = m_pointlight_config_data.atten_linear;
		light->attenuation.exp = m_pointlight_config_data.atten_exp;
		light->max_distance = m_pointlight_config_data.max_distance;
		light->color = m_pointlight_config_data.color;
		light->transform.SetPosition(m_pointlight_config_data.pos);
	}

	void EditorLayer::DisplaySceneData() {

		unsigned int total_vertices = 0;
		unsigned int total_lights = 0;


		for (const auto& group : m_active_scene->GetGroupMeshEntities()) {
			//total_vertices += group->GetInstances() * group->GetMeshData()->GetIndicesCount();
		}

		m_scene_data.total_vertices = total_vertices;
		m_scene_data.num_lights = total_lights;

		ImGui::Text("SCENE INFO");
		ImGui::Text(std::format("VERTEX COUNT: {}", m_scene_data.total_vertices).c_str());
		ImGui::Text(std::format("LIGHT COUNT: {}", m_scene_data.num_lights).c_str());
	};

}