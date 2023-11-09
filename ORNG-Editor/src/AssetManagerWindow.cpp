#include "pch/pch.h"
#include <imgui.h>
#include "AssetManagerWindow.h"
#include "assets/AssetManager.h"
#include "core/Window.h"
#include "util/ExtraUI.h"
#include "rendering/MeshAsset.h"
#include "core/CodedAssets.h"
#include "Icons.h"
#include "rendering/SceneRenderer.h"
#include "scene/SceneEntity.h"
#include "scene/SceneSerializer.h"
#include "imgui/misc/cpp/imgui_stdlib.h"


namespace ORNG {
	void GenerateErrorMessage(const std::string&) {
	}

	inline static std::string GenerateMeshBinaryPath(MeshAsset* p_mesh) {
		return ".\\res\\meshes\\" + p_mesh->filepath.substr(p_mesh->filepath.find_last_of("\\") + 1) + ".bin";
	}

	inline static std::string GenerateAudioFileClonePath(std::string og_path) {
		return ".\\res\\audio\\" + og_path.substr(og_path.rfind("\\") + 1);
	}

	void AssetManagerWindow::Init() {
		mp_preview_scene = std::make_unique<Scene>();

		// Setup preview scene used for viewing materials on meshes
		mp_preview_scene->LoadScene("");
		mp_preview_scene->post_processing.bloom.intensity = 0.25;
		auto& cube_entity = mp_preview_scene->CreateEntity("Cube");
		cube_entity.AddComponent<MeshComponent>();

		auto& cam_entity = mp_preview_scene->CreateEntity("Cam");
		auto* p_cam = cam_entity.AddComponent<CameraComponent>();
		p_cam->fov = 60.f;
		glm::vec3 cam_pos{ 3, 3, -3.0 };
		cam_entity.GetComponent<TransformComponent>()->SetPosition(cam_pos);
		mp_preview_scene->directional_light.SetLightDirection({ 0.1, 0.3, -1.0 });
		cam_entity.GetComponent<TransformComponent>()->LookAt({ 0, 0, 0 });
		p_cam->aspect_ratio = 1;
		p_cam->MakeActive();
		mp_preview_scene->Update(0);
		//mp_preview_scene->skybox.LoadEnvironmentMap(m_executable_directory + "/res/textures/AdobeStock_247957406.jpeg");

		m_asset_preview_spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
		m_asset_preview_spec.mag_filter = GL_LINEAR;
		m_asset_preview_spec.width = 256;
		m_asset_preview_spec.height = 256;
		m_asset_preview_spec.generate_mipmaps = true;
		m_asset_preview_spec.format = GL_RGBA;
		m_asset_preview_spec.internal_format = GL_RGBA16F;
		m_asset_preview_spec.storage_type = GL_FLOAT;
		m_asset_preview_spec.wrap_params = GL_CLAMP_TO_EDGE;

		m_current_2d_tex_spec = m_asset_preview_spec;
		m_current_2d_tex_spec.storage_type = GL_UNSIGNED_BYTE;

		CreateMaterialPreview(AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID));
		CreateMeshPreview(AssetManager::GetAsset<MeshAsset>(ORNG_BASE_MESH_ID));


		m_asset_listener.OnEvent = [this](const Events::AssetEvent& t_event) {
			OnProjectEvent(t_event);
			};
		Events::EventManager::RegisterListener(m_asset_listener);
	}


	void AssetManagerWindow::OnMainRender() {
		// Generate previews at this stage, delayed as if done during the ImGui rendering phase ImGui will stop rendering on that frame, causing a UI flicker
		for (auto* p_material : m_materials_to_gen_previews) {
			CreateMaterialPreview(p_material);
		}

		m_materials_to_gen_previews.clear();
	}


	void AssetManagerWindow::RenderConfirmationWindow(ConfirmationWindowData& data, int index) {
		ImVec2 window_size{ 600, 200 };
		ImGui::SetNextWindowSize(window_size);
		ImGui::SetNextWindowPos(ImVec2((Window::GetWidth() - window_size.x) / 2.0, (Window::GetHeight() - window_size.y) / 2.0));
		if (ImGui::Begin("Confirm")) {
			ImGui::SeparatorText("Confirm");
			ImGui::Text(data.str.c_str());

			if (ExtraUI::ColoredButton("No", ImVec4(0.5, 0, 0, 1)))
				m_confirmation_window_stack.erase(m_confirmation_window_stack.begin() + index);

			ImGui::SameLine();

			if (ExtraUI::ColoredButton("Yes", ImVec4(0, 0.5, 0, 1))) {
				data.callback();
				m_confirmation_window_stack.erase(m_confirmation_window_stack.begin() + index);
			}
		}
		ImGui::End();
	}

	void AssetManagerWindow::OnRenderUI() {
		for (auto uuid : m_asset_deletion_queue) {
			if (mp_selected_material && mp_selected_material->uuid() == uuid)
				mp_selected_material = nullptr;

			if (mp_selected_texture && mp_selected_texture->uuid() == uuid)
				mp_selected_texture = nullptr;

			AssetManager::DeleteAsset(uuid);
		}


		m_asset_deletion_queue.clear();
		RenderMainAssetWindow();

		if (mp_selected_material && RenderMaterialEditorSection())
			m_materials_to_gen_previews.push_back(mp_selected_material);

		if (mp_selected_texture)
			RenderTextureEditorSection();

		for (int i = m_confirmation_window_stack.size() - 1; i >= 0; i--) {
			RenderConfirmationWindow(m_confirmation_window_stack[i], i);
		}
	}


	void AssetManagerWindow::CreateAndSerializePrefab(uint64_t entt_id, const std::string& fp) {
		if (auto* p_entity = (*mp_scene_context)->GetEntity(entt_id)) {
			Prefab* prefab = AssetManager::AddAsset(new Prefab(fp));
			prefab->serialized_content = SceneSerializer::SerializeEntityIntoString(*p_entity);
			SceneSerializer::SerializeBinary(fp, *prefab);
		}
	}

	void AssetManagerWindow::RenderMainAssetWindow() {
		int window_width = Window::GetWidth() - 850;
		ImVec2 image_button_size = { glm::clamp(window_width / 8.f, 75.f, 150.f) , 150 };
		int column_count = glm::max((int)(window_width / (image_button_size.x + 40)), 1);
		ImGui::SetNextWindowSize(ImVec2(window_width, window_height));
		ImGui::SetNextWindowPos(AddImVec2(ImGui::GetMainViewport()->Pos, ImVec2(400, Window::GetHeight() - window_height)));

		ImGui::Begin("Assets", (bool*)0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
		ImGui::BeginTabBar("Selection");

		if (ImGui::BeginTabItem("Meshes")) // MESH TAB
		{
			if (ImGui::Button("Add mesh")) // MESH FILE EXPLORER
			{
				wchar_t valid_extensions[MAX_PATH] = L"Mesh Files: *.obj;*.fbx\0*.obj;*.fbx\0";

				//setting up file explorer callbacks
				std::function<void(std::string)> success_callback = [this](std::string filepath) {
					MeshAsset* asset = AssetManager::AddAsset(new MeshAsset(filepath));
					AssetManager::LoadMeshAsset(asset);
					};


				ExtraUI::ShowFileExplorer("", valid_extensions, success_callback);
			} // END MESH FILE EXPLORER

			static MeshAsset* p_dragged_mesh = nullptr;
			if (ImGui::BeginTable("Meshes", column_count)) // MESH VIEWING TABLE
			{
				for (auto* p_mesh_asset : AssetManager::GetView<MeshAsset>())
				{
					ImGui::PushID(p_mesh_asset);
					ImGui::TableNextColumn();

					std::string name = p_mesh_asset->filepath.substr(p_mesh_asset->filepath.find_last_of('\\') + 1);
					ExtraUI::NameWithTooltip(name);
					if (p_mesh_asset->GetLoadStatus())
					{
						ExtraUI::CenteredImageButton(ImTextureID(m_mesh_preview_textures[p_mesh_asset]->GetTextureHandle()), image_button_size);
						if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
							p_dragged_mesh = p_mesh_asset;
							ImGui::SetDragDropPayload("MESH", &p_dragged_mesh, sizeof(MeshAsset*));
							ImGui::EndDragDropSource();
						}
					}
					else
						ImGui::TextColored(ImVec4(1, 0, 0, 1), "Loading...");

					if (ExtraUI::RightClickPopup("mesh_popup"))
					{
						if (ImGui::Selectable("Delete")) {
							PushConfirmationWindow("Delete Mesh?", [this, p_mesh_asset] {
								// Cleanup binary file
								if (std::filesystem::exists(p_mesh_asset->filepath)) {
									std::filesystem::remove(p_mesh_asset->filepath);
								}
								m_asset_deletion_queue.push_back(p_mesh_asset->uuid());
								});
						}
						ImGui::EndPopup();
					}

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
					std::string new_filepath = ".\\res\\textures\\" + filepath.substr(filepath.find_last_of("\\") + 1);
					if (!std::filesystem::exists(new_filepath)) {
						FileCopy(filepath, new_filepath);
						m_current_2d_tex_spec.filepath = new_filepath;
						m_current_2d_tex_spec.generate_mipmaps = true;
						Texture2D* p_new_tex = new Texture2D(filepath);
						p_new_tex->SetSpec(m_current_2d_tex_spec);
						AssetManager::LoadTexture2D(AssetManager::AddAsset(p_new_tex));
						AssetManager::SerializeAssets();
					}
					else {
						ORNG_CORE_ERROR("Texture asset '{0}' not added, already found in project files", new_filepath);
					}
					};
				ExtraUI::ShowFileExplorer("", valid_extensions, success_callback);
			}


			static Texture2D* p_dragged_texture = nullptr;
			// Create table for textures
			if (ImGui::BeginTable("Textures", column_count)); // TEXTURE VIEWING TABLE
			{
				// Push textures into table
				for (auto* p_texture : AssetManager::GetView<Texture2D>())
				{
					ImGui::PushID(p_texture);
					ImGui::TableNextColumn();

					ExtraUI::NameWithTooltip(p_texture->GetSpec().filepath.substr(p_texture->GetSpec().filepath.find_last_of('\\') + 1).c_str());

					if (ExtraUI::CenteredImageButton(ImTextureID(p_texture->GetTextureHandle()), image_button_size)) {
						mp_selected_texture = p_texture;
						m_current_2d_tex_spec = mp_selected_texture->GetSpec();
					};

					if (ImGui::BeginDragDropSource()) {
						p_dragged_texture = p_texture;
						ImGui::SetDragDropPayload("TEXTURE", &p_dragged_texture, sizeof(Texture2D*));
						ImGui::EndDragDropSource();
					}

					if (ExtraUI::RightClickPopup("tex_popup"))
					{
						if (ImGui::Selectable("Delete")) { // Base material not deletable
							PushConfirmationWindow("Delete texture?", [p_texture] {
								if (std::filesystem::exists(p_texture->GetSpec().filepath)) {
									std::filesystem::remove(p_texture->GetSpec().filepath);
								}
								AssetManager::DeleteAsset(p_texture);
								});
						}
						ImGui::EndPopup();
					}

					ImGui::PopID();
				}

				ImGui::EndTable();
			} // END TEXTURE VIEWING TABLE
			ImGui::EndTabItem();
		} // END TEXTURE TAB


		if (ImGui::BeginTabItem("Materials")) { // MATERIAL TAB
			if (ImGui::IsItemClicked()) {
				// Refresh previews (they update automatically mostly but this fixes some bugs)
				for (auto& [name, p_asset] : AssetManager::Get().m_assets) {
					if (auto* p_mat = dynamic_cast<Material*>(p_asset)) {
						m_materials_to_gen_previews.push_back(p_mat);
					}
				}
			}

			if (ImGui::Button("Create material")) {
				auto* p_mat = new Material();
				AssetManager::AddAsset(p_mat);
			}

			if (ImGui::BeginTable("Material viewer", column_count)) { //MATERIAL VIEWING TABLE
				for (auto p_material : AssetManager::GetView<Material>()) {
					if (!m_material_preview_textures.contains(p_material))
						// No material preview so proceeding will lead to a crash
						continue;

					ImGui::TableNextColumn();
					ImGui::PushID(p_material);

					unsigned int tex_id = p_material->base_color_texture ? p_material->base_color_texture->GetTextureHandle() : AssetManager::GetAsset<Texture2D>(ORNG_BASE_TEX_ID)->GetTextureHandle();

					ExtraUI::NameWithTooltip(p_material->name.c_str());
					static Material* p_dragged_material = nullptr;

					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
						p_dragged_material = p_material;
						ImGui::SetDragDropPayload("MATERIAL", &p_dragged_material, sizeof(Material*));
						ImGui::EndDragDropSource();
					}
					if (ExtraUI::CenteredImageButton(ImTextureID(m_material_preview_textures[p_material]->GetTextureHandle()), image_button_size))
						mp_selected_material = p_material;

					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
						p_dragged_material = p_material;
						ImGui::SetDragDropPayload("MATERIAL", &p_dragged_material, sizeof(Material*));
						ImGui::EndDragDropSource();
					}

					// Deletion popup
					if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
						ImGui::OpenPopup("my_select_popup");
					}

					if (ImGui::BeginPopup("my_select_popup"))
					{
						if (p_material->uuid() != ORNG_BASE_MATERIAL_ID && ImGui::Selectable("Delete")) { // Base material not deletable
							// Process next frame before imgui render else imgui will attempt to render the preview texture for this which will be deleted
							m_asset_deletion_queue.push_back(p_material->uuid());
						}
						if (ImGui::Selectable("Duplicate")) {
							auto* p_new_material = new Material();
							*p_new_material = *p_material;
							// Give clone a unique UUID
							p_new_material->uuid = UUID();
							AssetManager::AddAsset(p_new_material);
							// Render a preview for material
							m_materials_to_gen_previews.push_back(p_new_material);
						}
						ImGui::EndPopup();
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
				std::string script_path = *mp_active_project_dir + "/res/scripts/" + std::to_string(UUID()()) + ".cpp";
				FileCopy(ORNG_CORE_MAIN_DIR "/src/scripting/ScriptingTemplate.cpp", script_path);
				std::string open_file_command = "start " + script_path;
				std::system(open_file_command.c_str());
			}

			if (ImGui::BeginTable("##script table", column_count)) {
				for (const auto& entry : std::filesystem::directory_iterator(*mp_active_project_dir + "\\res\\scripts")) {
					if (std::filesystem::is_regular_file(entry) && entry.path().extension().string() == ".cpp") {
						ImGui::TableNextColumn();

						std::string entry_path = entry.path().string();
						std::string relative_path = entry_path.substr(entry_path.rfind("\\res\\scripts") + 1);
						ImGui::PushID(entry_path.c_str());
						ExtraUI::NameWithTooltip(entry_path.substr(entry_path.find_last_of("\\") + 1));

						auto* p_asset = AssetManager::GetAsset<ScriptAsset>(relative_path);
						bool is_loaded = p_asset ? p_asset->symbols.loaded : false;
						if (p_asset && p_asset->symbols.loaded)
							ImGui::TextColored(ImVec4(0, 1, 0, 1), "Loaded");
						else if (p_asset && !p_asset->symbols.loaded)
							ImGui::TextColored(ImVec4(1, 0.2, 0, 1), "Loading failed");
						else
							ImGui::TextColored(ImVec4(1, 0, 0, 1), "Not loaded");

						ExtraUI::CenteredSquareButton(ICON_FA_FILE, image_button_size);
						static std::string dragged_script_filepath;
						if (is_loaded && ImGui::BeginDragDropSource()) {
							dragged_script_filepath = relative_path;
							ImGui::SetDragDropPayload("SCRIPT", &dragged_script_filepath, sizeof(std::string));
							ImGui::EndDragDropSource();
						}

						// Deletion popup
						if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
							ImGui::OpenPopup("script_option_popup");
						}

						if (ImGui::BeginPopup("script_option_popup"))
						{
							if ((!is_loaded && ImGui::Selectable("Load"))) {
								if (!AssetManager::AddAsset(new ScriptAsset(relative_path)))
									GenerateErrorMessage("AssetManager::AddScriptAsset failed");
							}
							else if (is_loaded && ImGui::Selectable("Reload")) {
								// Store all components that have this script as their symbols will need to be updated after the script reloads
								auto* p_curr_asset = AssetManager::GetAsset<ScriptAsset>(relative_path);
								std::vector<ScriptComponent*> components_to_reconnect;
								for (auto [entity, script_comp] : (*mp_scene_context)->m_registry.view<ScriptComponent>().each()) {
									if (p_curr_asset->PathEqualTo(script_comp.p_symbols->script_path))
										components_to_reconnect.push_back(&script_comp);
								}

								// Reload script and reconnect it to script components previously using it
								if (AssetManager::DeleteAsset(p_curr_asset)) {
									std::string dll_path = ScriptingEngine::GetDllPathFromScriptCpp(relative_path);
									std::optional<std::filesystem::file_status> existing_dll_status = std::filesystem::exists(dll_path) ? std::make_optional(std::filesystem::status(dll_path)) : std::nullopt;

									ScriptSymbols symbols = ScriptingEngine::GetSymbolsFromScriptCpp(relative_path);
									ScriptAsset* p_asset = nullptr;
									if (!symbols.loaded || (existing_dll_status.has_value() && std::filesystem::status(dll_path) == *existing_dll_status)) {
										GenerateErrorMessage("Failed to reload script");
									}

									p_asset = AssetManager::AddAsset(new ScriptAsset(symbols));

									// Reconnect script components that were using this script
									for (auto p_script : components_to_reconnect) {
										p_script->SetSymbols(&p_asset->symbols);
									}
								}
							}
							if (ImGui::Selectable("Delete")) {
								PushConfirmationWindow("Delete script asset?", [entry] {std::filesystem::remove(entry); });
							}
							if (ImGui::Selectable("Edit")) {
								std::string open_file_command = "start " + relative_path;
								std::system(open_file_command.c_str());
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


		if (ImGui::BeginTabItem("Audio")) {
			if (ImGui::Button("Add audio")) {
				wchar_t valid_extensions[MAX_PATH] = L"Audio Files: *.mp3;*.wav\0*.mp3;*.wav\0";

				//setting up file explorer callbacks
				std::function<void(std::string)> success_callback = [this](std::string filepath) {
					// Give relative path to current project directory
					std::string new_filepath{ GenerateAudioFileClonePath(filepath) };
					FileCopy(filepath, new_filepath);
					auto* p_sound = new SoundAsset(new_filepath);
					p_sound->source_filepath = new_filepath.substr(0, new_filepath.rfind(".osound"));
					AssetManager::AddAsset(p_sound);
					p_sound->CreateSound();
					};


				ExtraUI::ShowFileExplorer("", valid_extensions, success_callback);
			}

			if (ImGui::BeginTable("##audio asset table", column_count)) {
				for (auto [key, asset] : AssetManager::Get().m_assets) {
					auto* p_sound = dynamic_cast<SoundAsset*>(asset);
					if (!p_sound)
						continue;

					ImGui::TableNextColumn();
					ExtraUI::NameWithTooltip(p_sound->filepath);

					ExtraUI::CenteredSquareButton(ICON_FA_MUSIC, image_button_size);

					static SoundAsset* p_dragged_sound_asset;
					if (ImGui::BeginDragDropSource()) {
						p_dragged_sound_asset = p_sound;
						ImGui::SetDragDropPayload("AUDIO", &p_dragged_sound_asset, sizeof(SoundAsset*));
						ImGui::EndDragDropSource();
					}

					if (ExtraUI::RightClickPopup("audio popup")) {
						if (ImGui::Selectable("Delete")) {
							std::string path = p_sound->filepath;
							PushConfirmationWindow("Delete audio asset?", [p_sound, path] {
								AssetManager::DeleteAsset(p_sound);
								if (std::filesystem::exists(path))
									std::filesystem::remove(path);
								});
						}
						ImGui::EndPopup();
					}
				}
				ImGui::EndTable();
			}
			ImGui::EndTabItem();
		}


		if (ImGui::BeginTabItem("Prefabs")) {
			ImVec2 start_cursor_pos = ImGui::GetCursorPos();

			if (ImGui::BeginTable("##prefab table", column_count)) {
				for (auto p_prefab : AssetManager::GetView<Prefab>()) {
					ImGui::PushID(p_prefab);
					ImGui::TableNextColumn();
					ExtraUI::NameWithTooltip(p_prefab->filepath.substr(p_prefab->filepath.rfind("\\") + 1));
					ExtraUI::CenteredSquareButton(ICON_FA_FILE, image_button_size);
					static Prefab* p_dragged_prefab = nullptr;

					if (ImGui::BeginDragDropSource()) {
						p_dragged_prefab = p_prefab;
						ImGui::SetDragDropPayload("PREFAB", &p_dragged_prefab, sizeof(Prefab*));
						ImGui::EndDragDropSource();
					}

					if (ExtraUI::RightClickPopup("prefab-poup")) {
						if (ImGui::Selectable("Delete")) {
							uint64_t key = p_prefab->uuid();
							m_confirmation_window_stack.emplace_back("Delete prefab?", [=] {
								if (auto* p_curr_asset = AssetManager::GetAsset<Prefab>(key)) {
									if (std::filesystem::exists(p_curr_asset->filepath))
										std::filesystem::remove(p_curr_asset->filepath);

									AssetManager::DeleteAsset(p_curr_asset);
								}
								});
						}
						ImGui::EndPopup();
					}
					ImGui::PopID();
				}
				ImGui::EndTable();
			}

			ImGui::SetCursorPos(start_cursor_pos);
			ImGui::InvisibleButton("prefab-child", ImGui::GetContentRegionAvail());

			// Setup entity drag drop on entire section
			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("ENTITY")) {
					if (p_payload->DataSize == sizeof(std::vector<uint64_t>)) {
						std::vector<uint64_t>& id_vec = *static_cast<std::vector<uint64_t>*>(p_payload->Data);
						for (auto id : id_vec) {
							SceneEntity* p_entity = (*mp_scene_context)->GetEntity(id);
							std::string fp = *mp_active_project_dir + "\\res\\prefabs\\" + p_entity->name + ".opfb";

							if (auto* p_asset = AssetManager::GetAsset<Prefab>(fp)) {
								m_confirmation_window_stack.emplace_back(std::format("Overwrite prefab '{}'?", fp), [=] {AssetManager::DeleteAsset(p_asset); CreateAndSerializePrefab(p_entity->GetUUID(), fp); });
							}
							else
								CreateAndSerializePrefab(p_entity->GetUUID(), fp);
						}
					}
				}
			}
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
		ImGui::End();
	}



	void AssetManagerWindow::OnProjectEvent(const Events::AssetEvent& t_event) {
		switch (t_event.event_type) {
		case Events::AssetEventType::MESH_LOADED: {
			auto* p_mesh = reinterpret_cast<MeshAsset*>(t_event.data_payload);
			CreateMeshPreview(p_mesh);

			std::string filepath{ GenerateMeshBinaryPath(p_mesh) };
			if (!std::filesystem::exists(filepath) && filepath.substr(0, filepath.size() - 4).find(".bin") == std::string::npos) {
				// Gen binary file if none exists
				SceneSerializer::SerializeBinary(filepath, *p_mesh);
			}

			break;
		}
		case Events::AssetEventType::MATERIAL_LOADED:
		{
			auto* p_material = reinterpret_cast<Material*>(t_event.data_payload);
			m_materials_to_gen_previews.push_back(p_material);
			break;
		}
		case Events::AssetEventType::MATERIAL_DELETED:
		{
			auto* p_material = reinterpret_cast<Material*>(t_event.data_payload);

			// Check if the material is in a loading queue, if it is it needs to be removed
			auto it = std::ranges::find(m_materials_to_gen_previews, p_material);
			if (it != m_materials_to_gen_previews.end())
				m_materials_to_gen_previews.erase(it);

			m_material_preview_textures.erase(p_material);
			break;
		}
		case Events::AssetEventType::MESH_DELETED:
		{
			auto* p_mesh = reinterpret_cast<MeshAsset*>(t_event.data_payload);
			m_mesh_preview_textures.erase(p_mesh);



			break;
		}
		}
	}



	void AssetManagerWindow::CreateMeshPreview(MeshAsset* p_asset) {
		auto p_tex = std::make_shared<Texture2D>(p_asset->filepath.substr(p_asset->filepath.find_last_of("/") + 1) + " - Mesh preview");
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
		// Disable additional user renderpasses as this is just a preview of a mesh
		settings.do_intercept_renderpasses = false;

		SceneRenderer::SceneRenderingOutput output = SceneRenderer::RenderScene(settings);

		GL_StateManager::BindTexture(GL_TEXTURE_2D, p_tex->GetTextureHandle(), GL_TEXTURE0, true);
		glGenerateMipmap(GL_TEXTURE_2D);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, 0, GL_TEXTURE0, true);
	}

	void AssetManagerWindow::CreateMaterialPreview(const Material* p_material) {
		auto p_tex = std::make_shared<Texture2D>(p_material->name + " - Material preview");
		p_tex->SetSpec(m_asset_preview_spec);
		m_material_preview_textures[p_material] = p_tex;


		auto* p_mesh = mp_preview_scene->GetEntity("Cube")->GetComponent<MeshComponent>();
		if (auto* p_replacement_mesh = AssetManager::GetAsset<MeshAsset>(ORNG_BASE_MESH_ID); p_mesh->GetMeshData() != p_replacement_mesh)
			p_mesh->SetMeshAsset(p_replacement_mesh);

		mp_preview_scene->GetEntity("Cube")->GetComponent<TransformComponent>()->SetScale(1.0, 1.0, 1.0);

		mp_preview_scene->Update(0);
		for (int i = 0; i < p_mesh->GetMaterials().size(); i++) {
			p_mesh->SetMaterialID(i, p_material);
		}
		mp_preview_scene->Update(0);

		SceneRenderer::SceneRenderingSettings settings;
		SceneRenderer::SetActiveScene(&*mp_preview_scene);
		settings.p_output_tex = &*p_tex;
		// Disable additional user renderpasses as this is just a preview of a mesh
		settings.do_intercept_renderpasses = false;
		SceneRenderer::SceneRenderingOutput output = SceneRenderer::RenderScene(settings);

		GL_StateManager::BindTexture(GL_TEXTURE_2D, p_tex->GetTextureHandle(), GL_TEXTURE0, true);
		glGenerateMipmap(GL_TEXTURE_2D);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, 0, GL_TEXTURE0, true);
	}


	bool AssetManagerWindow::RenderMaterialEditorSection() {
		bool ret = false;

		if (ImGui::Begin("Material editor")) {
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Right)) {
				// Hide this tree node
				mp_selected_material = nullptr;
				goto window_end;
			}


			ImGui::Text("Name: ");
			ImGui::SameLine();
			ImGui::InputText("##name input", &mp_selected_material->name);
			ImGui::Spacing();

			ret |= RenderMaterialTexture("Base", mp_selected_material->base_color_texture);
			ret |= RenderMaterialTexture("Normal", mp_selected_material->normal_map_texture);
			ret |= RenderMaterialTexture("Roughness", mp_selected_material->roughness_texture);
			ret |= RenderMaterialTexture("Metallic", mp_selected_material->metallic_texture);
			ret |= RenderMaterialTexture("AO", mp_selected_material->ao_texture);
			ret |= RenderMaterialTexture("Displacement", mp_selected_material->displacement_texture);
			ret |= RenderMaterialTexture("Emissive", mp_selected_material->emissive_texture);

			ImGui::Text("Colors");
			ImGui::Spacing();
			ret |= ExtraUI::ShowVec4Editor("Base color", mp_selected_material->base_color);
			if (mp_selected_material->base_color.w < 0.0 || mp_selected_material->base_color.w > 1.0)
				mp_selected_material->base_color.w = 1.0;

			if (!mp_selected_material->roughness_texture)
				ret |= ImGui::SliderFloat("Roughness", &mp_selected_material->roughness, 0.f, 1.f);

			if (!mp_selected_material->metallic_texture)
				ret |= ImGui::SliderFloat("Metallic", &mp_selected_material->metallic, 0.f, 1.f);

			if (!mp_selected_material->ao_texture)
				ret |= ImGui::SliderFloat("AO", &mp_selected_material->ao, 0.f, 1.f);

			ImGui::Checkbox("Emissive", &mp_selected_material->emissive);


			if (mp_selected_material->emissive || mp_selected_material->emissive_texture)
				ret |= ImGui::SliderFloat("Emissive strength", &mp_selected_material->emissive_strength, -10.f, 10.f);

			int num_parallax_layers = mp_selected_material->parallax_layers;
			if (mp_selected_material->displacement_texture) {
				ret |= ImGui::InputInt("Parallax layers", &num_parallax_layers);

				if (num_parallax_layers >= 0)
					mp_selected_material->parallax_layers = num_parallax_layers;

				ret |= ImGui::InputFloat("Parallax scale", &mp_selected_material->parallax_height_scale);
			}

			ret |= ExtraUI::ShowVec2Editor("Tile scale", mp_selected_material->tile_scale);

			ImGui::Text("Render group");
			const char* render_groups[2] = { "Solid", "Alpha Tested" };
			static const char* current_item = nullptr;
			current_item = mp_selected_material->render_group == SOLID ? render_groups[0] : render_groups[1];
			if (ImGui::BeginCombo("##rg", current_item)) {
				for (int i = 0; i < 2; i++) {
					bool is_selected = current_item == render_groups[i];
					if (ImGui::Selectable(render_groups[i], is_selected)) {
						mp_selected_material->render_group = i == 0 ? SOLID : ALPHA_TESTED;
					}
				}

				ImGui::EndCombo();
			}
		}
	window_end:
		ImGui::End();
		return ret;
	}



	bool AssetManagerWindow::RenderMaterialTexture(const char* name, Texture2D*& p_tex) {
		bool ret = false;
		ImGui::PushID(p_tex);
		if (p_tex) {
			ImGui::Text(std::format("{} texture - {}", name, p_tex->GetName()).c_str());
			if (ImGui::ImageButton(ImTextureID(p_tex->GetTextureHandle()), ImVec2(75, 75), ImVec2(0, 1), ImVec2(1, 0))) {
				mp_selected_texture = p_tex;
				m_current_2d_tex_spec = p_tex->GetSpec();
				ret = true;
			};
		}
		else {
			ImGui::Text(std::format("{} texture - NONE", name).c_str());
			ret |= ImGui::ImageButton(ImTextureID(0), ImVec2(75, 75));
		}

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("TEXTURE")) {
				if (p_payload->DataSize == sizeof(Texture2D*))
					p_tex = *static_cast<Texture2D**>(p_payload->Data);
			}
			ImGui::EndDragDropTarget();
			ret = true;
		}

		if (ImGui::IsItemHovered()) {
			if (ImGui::IsMouseClicked(1)) {
				// Delete texture from material
				p_tex = nullptr;
				ret = true;
			}
		}

		ImGui::PopID();
		return ret;
	}




	void AssetManagerWindow::RenderTextureEditorSection() {
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

				m_current_2d_tex_spec.generate_mipmaps = true;

				if (ImGui::Button("Load")) {
					mp_selected_texture->SetSpec(m_current_2d_tex_spec);
					mp_selected_texture->LoadFromFile();
				}

				ImGui::TableNextColumn();
				float size = glm::min(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
				ExtraUI::CenteredImageButton(ImTextureID(mp_selected_texture->GetTextureHandle()), ImVec2(size, size));
				ImGui::Spacing();
				ImGui::EndTable();
			}
		}

	window_end:
		ImGui::End();
	}
}