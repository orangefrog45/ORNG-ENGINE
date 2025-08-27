#include "pch/pch.h"
#include <shellapi.h>
#include <imgui.h>
#include <yaml-cpp/yaml.h>

#include "AssetManagerWindow.h"

#include "EditorLayer.h"
#include "Icons.h"
#include "components/systems/EnvMapSystem.h"
#include "rendering/renderpasses/BloomPass.h"
#include "rendering/renderpasses/DepthPass.h"
#include "rendering/renderpasses/FogPass.h"
#include "rendering/renderpasses/PostProcessPass.h"
#include "rendering/renderpasses/SSAOPass.h"
#include "rendering/renderpasses/GBufferPass.h"
#include "rendering/renderpasses/LightingPass.h"
#include "rendering/renderpasses/TransparencyPass.h"
#include "assets/Prefab.h"
#include "assets/PhysXMaterialAsset.h"
#include "components/PhysicsComponent.h"
#include "assets/AssetManager.h"
#include "core/Window.h"
#include "util/ExtraUI.h"
#include "rendering/MeshAsset.h"
#include "scene/SceneEntity.h"
#include "scene/SceneSerializer.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "core/GLStateManager.h"
#include "physics/Physics.h"
#include "components/ComponentSystems.h"

using namespace ORNG;

void AssetManagerWindow::InitPreviewScene() {
	mp_preview_scene = std::make_unique<Scene>();

	mp_preview_scene->AddSystem(new MeshInstancingSystem(&*mp_preview_scene), 0)->OnLoad();
	mp_preview_scene->AddSystem(new SpotlightSystem(&*mp_preview_scene), 0)->OnLoad();
	mp_preview_scene->AddSystem(new PointlightSystem(&*mp_preview_scene), 0)->OnLoad();
	mp_preview_scene->AddSystem(new CameraSystem(&*mp_preview_scene), 1)->OnLoad();
	mp_preview_scene->AddSystem(new SceneUBOSystem(&*mp_preview_scene), 2)->OnLoad();
	mp_preview_scene->AddSystem(new EnvMapSystem(&*mp_preview_scene), 2); // Don't load this as the serialization hooks will intefere with the main scene
	auto& env_map_system = mp_preview_scene->GetSystem<EnvMapSystem>();
	env_map_system.skybox.using_env_map = true;
	env_map_system.LoadSkyboxFromHDRFile(GetApplicationExecutableDirectory() + "/res/textures/preview-sky.hdr", 512);

	mp_preview_scene->post_processing.bloom.intensity = 0.25;
	auto& cube_entity = mp_preview_scene->CreateEntity("Sphere");
	cube_entity.AddComponent<MeshComponent>();

	auto& cam_entity = mp_preview_scene->CreateEntity("Cam");
	auto* p_cam = cam_entity.AddComponent<CameraComponent>();
	p_cam->fov = 60.f;

	mp_preview_scene->directional_light.SetLightDirection({ 0.1, 0.3, -1.0 });
	cam_entity.GetComponent<TransformComponent>()->SetPosition({ 3, 3, -3.0 });
	cam_entity.GetComponent<TransformComponent>()->LookAt({ 0, 0, 0 });

	p_cam->aspect_ratio = 1.f;
	p_cam->MakeActive();

	mp_preview_scene->GetSystem<MeshInstancingSystem>().OnUpdate();

	m_asset_preview_spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
	m_asset_preview_spec.mag_filter = GL_LINEAR;
	m_asset_preview_spec.width = 2048;
	m_asset_preview_spec.height = 2048;
	m_asset_preview_spec.generate_mipmaps = true;
	m_asset_preview_spec.format = GL_RGBA;
	m_asset_preview_spec.internal_format = GL_RGBA16F;
	m_asset_preview_spec.storage_type = GL_FLOAT;
	m_asset_preview_spec.wrap_params = GL_CLAMP_TO_EDGE;

	m_preview_render_target.SetSpec(m_asset_preview_spec);

	m_preview_render_graph.AddRenderpass<DepthPass>();
	m_preview_render_graph.AddRenderpass<GBufferPass>();
	m_preview_render_graph.AddRenderpass<SSAOPass>();
	m_preview_render_graph.AddRenderpass<LightingPass>();
	m_preview_render_graph.AddRenderpass<FogPass>();
	m_preview_render_graph.AddRenderpass<TransparencyPass>();
	m_preview_render_graph.AddRenderpass<BloomPass>();
	m_preview_render_graph.AddRenderpass<PostProcessPass>();
	m_preview_render_graph.SetData("OutCol", &m_preview_render_target);
	m_preview_render_graph.SetData("BloomInCol", &m_preview_render_target);
	m_preview_render_graph.SetData("PPS", &mp_preview_scene->post_processing);
	m_preview_render_graph.SetData("Scene", mp_preview_scene.get());
	m_preview_render_graph.Init();
}


void AssetManagerWindow::Init() {
	InitPreviewScene();

	m_current_2d_tex_spec = m_asset_preview_spec;
	m_current_2d_tex_spec.storage_type = GL_UNSIGNED_BYTE;


	m_asset_listener.OnEvent = [this](const Events::AssetEvent& t_event) {
		OnProjectEvent(t_event);
		};
	Events::EventManager::RegisterListener(m_asset_listener);

	for (auto* p_mesh : AssetManager::GetView<MeshAsset>()) {
		m_meshes_to_gen_previews.push_back(p_mesh);
	}

	for (auto* p_mat : AssetManager::GetView<Material>()) {
		m_materials_to_gen_previews.push_back(p_mat);
	}
}


void AssetManagerWindow::OnMainRender() {
	// Generate previews at this stage, delayed as if done during the ImGui rendering phase ImGui will stop rendering on that frame, causing a UI flicker
	if (!m_materials_to_gen_previews.empty()) {
		CreateMaterialPreview(m_materials_to_gen_previews[0]);
		m_materials_to_gen_previews.erase(m_materials_to_gen_previews.begin());
	}
	else if (!m_meshes_to_gen_previews.empty()) {
		CreateMeshPreview(m_meshes_to_gen_previews[0]);
		m_meshes_to_gen_previews.erase(m_meshes_to_gen_previews.begin());
	}

}


void AssetManagerWindow::RenderConfirmationWindow(ConfirmationWindowData& data, int index) {
	ImVec2 window_size{ 600, 200 };
	ImGui::SetNextWindowSize(window_size);
	ImGui::SetNextWindowPos(ImVec2((Window::GetWidth() - window_size.x) / 2.0, (Window::GetHeight() - window_size.y) / 2.0));
	if (ImGui::Begin("Confirm")) {
		ImGui::SeparatorText("Confirm");
		ImGui::Text(data.str.c_str());

		if (data.imgui_render) data.imgui_render();

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

void AssetManagerWindow::ProcessAssetDeletionQueue() {
	if (m_asset_deletion_queue.empty()) return;

	// This is a bit hacky, but a big time-saver.
	// If an asset gets deleted, all references of it need to be removed from every component type.
	// Normally, this would mean iterating manually over every affected component type, checking the required fields, replacing, etc.
	// Instead, here the scene just gets temporarily serialized in its current state, the asset is then deleted, and the temporary scene deserialized.
	// Because valid UUID checks are implemented in deserialization code, any now-invalid UUIDs are automatically replaced during deserialization.
	// Thus we get the exact same scene, but any references to the just-deleted asset(s) have been replaced thanks to the deserializer.
	// Obviously not as performant as the manual way, but generally still fast, and much more maintainable, as I only have to ensure deserialization is robust.
	std::string ser;
	SceneSerializer::SerializeScene(*mp_scene_context, ser, true);
	mp_scene_context->ClearAllEntities(false);

	for (uint64_t uuid : m_asset_deletion_queue) {
		if (mp_selected_material && mp_selected_material->uuid() == uuid)
			mp_selected_material = nullptr;

		if (mp_selected_texture && mp_selected_texture->uuid() == uuid)
			mp_selected_texture = nullptr;

		AssetManager::DeleteAsset(uuid);
	}

	SceneSerializer::DeserializeScene(*mp_scene_context, ser, false);

	m_asset_deletion_queue.clear();
}


void AssetManagerWindow::OnRenderUI() {
	ProcessAssetDeletionQueue();
	RenderMainAssetWindow();

	if (mp_selected_material && mp_selected_material->uuid() != static_cast<uint64_t>(BaseAssetIDs::DEFAULT_MATERIAL) && RenderMaterialEditorSection())
		m_materials_to_gen_previews.push_back(mp_selected_material);

	if (mp_selected_texture)
		RenderTextureEditorSection();

	if (mp_selected_physx_material)
		RenderPhysXMaterialEditor();

	for (int i = m_confirmation_window_stack.size() - 1; i >= 0; i--) {
		RenderConfirmationWindow(m_confirmation_window_stack[i], i);
	}
}


bool AssetManagerWindow::CreateAndSerializePrefab(SceneEntity& entity, const std::string& fp, uint64_t uuid) {
	std::vector<SceneEntity*> entities;
	entities.push_back(&entity);

	entity.ForEachChildRecursive([&](entt::entity child_handle) {
		entities.push_back(mp_scene_context->GetEntity(child_handle));
		});

	Scene::SortEntitiesNumParents(entities, false);

	Prefab* prefab = AssetManager::AddAsset(new Prefab(fp));
	if (uuid != 0) prefab->uuid = UUID<uint64_t>{ uuid };

	prefab->serialized_content = SceneSerializer::SerializeEntityArrayIntoString(entities);
	prefab->node = YAML::Load(prefab->serialized_content);
	AssetManager::GetSerializer().SerializeAssetToBinaryFile(*prefab, fp);

	return true;
}

void AssetManagerWindow::UnloadScriptFromComponents(const std::string& relative_path) {
	auto* p_curr_asset = AssetManager::GetAsset<ScriptAsset>(relative_path);
	for (auto [entity, script_comp] : mp_scene_context->m_registry.view<ScriptComponent>().each()) {
		if (auto* p_symbols = script_comp.GetSymbols(); p_symbols && p_curr_asset->symbols.script_name == script_comp.GetSymbols()->script_name) {
			// Free memory for instances that were allocated by the DLL
			script_comp.GetSymbols()->DestroyInstance(script_comp.p_instance);
			script_comp.p_instance = nullptr;
		}
	}
}

void AssetManagerWindow::LoadScript(ScriptAsset& asset, const std::string& relative_path) {
	asset.symbols = ScriptingEngine::GetSymbolsFromScriptCpp(relative_path);

	for (auto [entity, script] : mp_scene_context->GetRegistry().view<ScriptComponent>().each()) {
		if (script.GetSymbols() && script.GetSymbols()->script_name == asset.symbols.script_name) {
			script.SetSymbols(&asset.symbols);
		}
	}
}

void AssetManagerWindow::UnloadScript(ScriptAsset& script) {
	UnloadScriptFromComponents(script.filepath);
	ScriptingEngine::UnloadScriptDLL(script.filepath);
	script.symbols.loaded = false;
}

void AssetManagerWindow::RenderScriptAsset(ScriptAsset* p_asset) {
	AssetDisplaySpec spec;

	unsigned using_count = 0;
	for (const auto& [entity, script] : mp_scene_context->m_registry.view<ScriptComponent>().each()) {
		if (script.GetSymbols() == &p_asset->symbols)
			using_count++;
	}

	spec.delete_confirmation_str = using_count == 0 ? "" : std::format("{} entities are using this.", using_count);

	spec.on_drag = [p_asset]() {
		static ScriptAsset* p_dragged_script = nullptr;
		if (!p_asset->symbols.loaded) {
			ImGui::EndDragDropSource();
			return;
		}

		p_dragged_script = p_asset;
		ImGui::SetDragDropPayload("SCRIPT", &p_dragged_script, sizeof(ScriptAsset*));
		ImGui::EndDragDropSource();
		};

	spec.on_delete = [p_asset, this]() {
		UnloadScriptFromComponents(p_asset->filepath);
		ScriptingEngine::OnDeleteScript(p_asset->filepath);
		};

	if (!p_asset->symbols.loaded) {
		spec.popup_spec.options.push_back(std::make_pair("Load",
			[p_asset, this]() {
				LoadScript(*p_asset, p_asset->filepath);
			}));
	} else {

	spec.popup_spec.options.push_back(std::make_pair("Unload",
		[this, p_asset]() {
			UnloadScript(*p_asset);
		}));
	};

	spec.popup_spec.options.push_back(std::make_pair("Edit",
		[p_asset]() { ShellExecute(NULL, "open", p_asset->filepath.c_str(), NULL, NULL, SW_SHOWDEFAULT); }
	));

	spec.p_tex = AssetManager::GetAsset<Texture2D>(static_cast<uint64_t>(BaseAssetIDs::WHITE_TEXTURE));

	if (!ScriptingEngine::GetScriptData(p_asset->filepath).is_loaded) {
		ImGui::PushStyleColor(ImGuiCol_Text, { 1, 0.5, 0, 1 });
		RenderBaseAsset(p_asset, spec);
		ImGui::PopStyleColor();
	}
	else {
		RenderBaseAsset(p_asset, spec);
	}

}


void AssetManagerWindow::RenderPhysXMaterial(PhysXMaterialAsset* p_material) {
	AssetDisplaySpec spec;

	unsigned using_count = 0;
	for (auto [entity, comp] : mp_scene_context->m_registry.view<PhysicsComponent>().each()) {
		if (comp.GetMaterial() == p_material)
			using_count++;
	}

	spec.delete_confirmation_str = using_count == 0 ? "" : std::format("{} entities are using this.", using_count);

	spec.on_drag = [p_material]() {
		static PhysXMaterialAsset* p_dragged_material = nullptr;
		p_dragged_material = p_material;
		ImGui::SetDragDropPayload("PHYSX-MATERIAL", &p_dragged_material, sizeof(PhysXMaterialAsset*));
		ImGui::EndDragDropSource();
		};

	spec.on_click = [this, p_material]() {
		mp_selected_physx_material = p_material;
		};
	spec.p_tex = AssetManager::GetAsset<Texture2D>(static_cast<uint64_t>(BaseAssetIDs::WHITE_TEXTURE));

	RenderBaseAsset(p_material, spec);
}



void AssetManagerWindow::RenderMeshAsset(MeshAsset* p_mesh_asset) {
	AssetDisplaySpec spec;

	unsigned using_count = 0;
	for (auto [entity, mesh] : mp_scene_context->m_registry.view<MeshComponent>().each()) {
		if (mesh.GetMeshData() == p_mesh_asset)
			using_count++;
	}

	spec.delete_confirmation_str = using_count == 0 ? "" : std::format("{} entities are using this.", using_count);

	spec.on_drag = [p_mesh_asset]() {
		static MeshAsset* p_dragged_mesh = nullptr;

		p_dragged_mesh = p_mesh_asset;
		if (!p_dragged_mesh->GetLoadStatus()) {
			ImGui::EndDragDropSource();
			return;
		}

		ImGui::SetDragDropPayload("MESH", &p_dragged_mesh, sizeof(MeshAsset*));
		ImGui::EndDragDropSource();
	};

	spec.p_tex = m_mesh_preview_textures.contains(p_mesh_asset) ? m_mesh_preview_textures[p_mesh_asset].get() : AssetManager::GetAsset<Texture2D>(static_cast<uint64_t>(BaseAssetIDs::WHITE_TEXTURE));

	RenderBaseAsset(p_mesh_asset, spec);
}

void AssetManagerWindow::RenderSceneAsset(SceneAsset* p_asset) {
	AssetDisplaySpec spec{};

	spec.delete_confirmation_str = "This will delete the entire scene and is not reversible.";

	spec.on_drag = [p_asset]() {
		static SceneAsset* p_dragged_scene = nullptr;
		p_dragged_scene = p_asset;
		ImGui::SetDragDropPayload("SCENE ASSET", &p_dragged_scene, sizeof(SceneAsset*));
		ImGui::EndDragDropSource();
	};

	spec.on_click = [this, p_asset]() {
		mp_selected_scene = p_asset;
	};

	// Is scene active in the editor
	const bool is_active = mp_editor->mp_scene_context->m_asset_uuid() == p_asset->uuid();
	if (is_active) {
		spec.popup_spec.options.emplace_back("Active", [this, p_asset]() {});
	} else {
		spec.popup_spec.options.emplace_back("Make active", [this, p_asset]() {
			PushConfirmationWindow("Make scene active? You will lose any unsaved changes on the current scene.", [p_asset]() {
				Events::EventManager::DispatchEvent(SwitchSceneEvent{p_asset});
			});
		});
	}

	spec.allow_deletion = !is_active;
	spec.p_tex = AssetManager::GetAsset<Texture2D>(static_cast<uint64_t>(BaseAssetIDs::WHITE_TEXTURE));
	RenderBaseAsset(p_asset, spec);
}

void AssetManagerWindow::RenderBaseAsset(Asset* p_asset, const AssetDisplaySpec& display_spec) {
	ImGui::PushID(p_asset);

	ExtraUI::NameWithTooltip(display_spec.override_name.empty() ? GetFilename(p_asset->filepath).c_str() : display_spec.override_name);

	if (ExtraUI::CenteredImageButton(ImTextureID(display_spec.p_tex->GetTextureHandle()), image_button_size) && display_spec.on_click) {
		display_spec.on_click();
	};

	if (ImGui::BeginDragDropSource() && display_spec.on_drag) {
		display_spec.on_drag();
	}

	if (ExtraUI::RightClickPopup("asset_popup"))
	{
		if (display_spec.allow_deletion && ImGui::Selectable("Delete"))
			OnRequestDeleteAsset(p_asset, display_spec.delete_confirmation_str, display_spec.on_delete);

		for (auto& pair : display_spec.popup_spec.options) {
			if (ImGui::Selectable(pair.first))
				pair.second();
		}

		ImGui::EndPopup();
	}

	ImGui::PopID();
}



void AssetManagerWindow::RenderTexture(Texture2D* p_texture) {
	AssetDisplaySpec spec;

	spec.on_drag = [p_texture]() {
		static Texture2D* p_dragged_texture = nullptr;
		p_dragged_texture = p_texture;
		ImGui::SetDragDropPayload("TEXTURE", &p_dragged_texture, sizeof(Texture2D*));
		ImGui::EndDragDropSource();
		};

	spec.on_delete = [p_texture]() {
		ORNG_CORE_TRACE("DELETING {0}", p_texture->uuid());
		FileDelete(p_texture->GetSpec().filepath + ".otex");
		};

	spec.on_click = [this, p_texture]() {
		mp_selected_texture = p_texture;
		m_current_2d_tex_spec = mp_selected_texture->GetSpec();
		};

	spec.p_tex = p_texture;

	RenderBaseAsset(p_texture, spec);
}


void AssetManagerWindow::RenderMaterial(Material* p_material) {
	AssetDisplaySpec spec;
	spec.on_drag = [p_material]() {
		static Material* p_dragged_material = nullptr;
		p_dragged_material = p_material;
		ImGui::SetDragDropPayload("MATERIAL", &p_dragged_material, sizeof(Material*));
		ImGui::EndDragDropSource();
		};

	spec.on_click = [this, p_material]() {
		mp_selected_material = p_material;
		};

	std::function<void()> on_duplicate = [this, p_material]() {
		auto* p_new_material = new Material();
		*p_new_material = *p_material;
		// Give clone a unique UUID
		p_new_material->uuid = UUID<uint64_t>();
		p_new_material->filepath = ReplaceFileExtension(p_new_material->filepath, " - Copy.omat");
		AssetManager::AddAsset(p_new_material);
		// Render a preview for material
		m_materials_to_gen_previews.push_back(p_new_material);
		};

	spec.popup_spec.options.push_back(std::make_pair("Duplicate", on_duplicate));
	spec.p_tex = m_material_preview_textures.contains(p_material) ? m_material_preview_textures.at(p_material).get() : AssetManager::GetAsset<Texture2D>(static_cast<uint64_t>(BaseAssetIDs::WHITE_TEXTURE));
	spec.override_name = p_material->name;

	RenderBaseAsset(p_material, spec);
}


void AssetManagerWindow::RenderAudioAsset(SoundAsset* p_asset) {
	AssetDisplaySpec spec;

	spec.on_drag = [p_asset]() {
		static SoundAsset* p_dragged_sound_asset;
		p_dragged_sound_asset = p_asset;
		ImGui::SetDragDropPayload("AUDIO", &p_dragged_sound_asset, sizeof(SoundAsset*));
		ImGui::EndDragDropSource();
		};

	spec.p_tex = AssetManager::GetAsset<Texture2D>(static_cast<uint64_t>(BaseAssetIDs::WHITE_TEXTURE));

	RenderBaseAsset(p_asset, spec);
}

void AssetManagerWindow::RenderPrefab(Prefab* p_prefab) {
	AssetDisplaySpec spec;

	spec.on_drag = [p_prefab]() {
		static Prefab* p_dragged_prefab = nullptr;
		p_dragged_prefab = p_prefab;
		ImGui::SetDragDropPayload("PREFAB", &p_dragged_prefab, sizeof(Prefab*));
		ImGui::EndDragDropSource();
		};

	spec.p_tex = AssetManager::GetAsset<Texture2D>(static_cast<uint64_t>(BaseAssetIDs::WHITE_TEXTURE));

	RenderBaseAsset(p_prefab, spec);
}

void AssetManagerWindow::RenderAsset(Asset *p_asset) {
	if (auto* p_casted = dynamic_cast<Texture2D*>(p_asset))
		RenderTexture(p_casted);
	else if (auto* p_casted = dynamic_cast<MeshAsset*>(p_asset))
		RenderMeshAsset(p_casted);
	else if (auto* p_casted = dynamic_cast<SoundAsset*>(p_asset))
		RenderAudioAsset(p_casted);
	else if (auto* p_casted = dynamic_cast<Prefab*>(p_asset))
		RenderPrefab(p_casted);
	else if (auto* p_casted = dynamic_cast<Material*>(p_asset))
		RenderMaterial(p_casted);
	else if (auto* p_casted = dynamic_cast<PhysXMaterialAsset*>(p_asset))
		RenderPhysXMaterial(p_casted);
	else if (auto* p_casted = dynamic_cast<ScriptAsset*>(p_asset))
		RenderScriptAsset(p_casted);
	else if (auto* p_casted = dynamic_cast<SceneAsset*>(p_asset))
		RenderSceneAsset(p_casted);
}

bool AssetManagerWindow::RenderDirectory(const std::filesystem::path& path, std::string& active_path) {
	static bool ret = false;

	ImGui::Text(path.filename().generic_string().c_str());
	ImGui::Button(ICON_FA_FOLDER_CLOSED, image_button_size);

	if (ExtraUI::RightClickPopup((std::string{"Directory settings"} + path.generic_string()).c_str())) {
		static std::string s;
		ImGui::InputText("##directory settings input", &s);

		if (ImGui::Selectable("Delete")) {
			PushConfirmationWindow(std::format("Delete directory? This will delete any assets inside, and cannot be undone.", s), [this, path] {
				for (auto& [uuid, p_asset] : AssetManager::Get().m_assets) {
					if (uuid >= static_cast<uint64_t>(BaseAssetIDs::NUM_BASE_ASSETS) && IsFilepathAChildOf(p_asset->filepath, path)) {
						m_asset_deletion_queue.push_back(uuid);
					}
				}

				FileDelete(path.generic_string());
				ret = true;
			});
		}

		if (ImGui::Selectable("Rename")) {
			PushConfirmationWindow(std::format("Rename directory to '{}'?", s), [path] {
				try {
					std::string old_path = path.generic_string();
					std::string new_path = path.parent_path().generic_string() + '/' + s;
					std::filesystem::rename(path, new_path);
					for (auto& [uuid, asset] : AssetManager::Get().m_assets) {
						if (!IsFilepathAChildOf(asset->filepath, path)) continue;

						StringReplace(asset->filepath, old_path, new_path);
					}
				} catch(std::exception& e) {
					ORNG_CORE_ERROR("Failed to rename directory: {}", e.what());
				}
			});
		}

		ImGui::EndPopup();
	}

	if (ImGui::IsItemClicked()) {
		active_path = path.generic_string();
		return true;
	}

	bool ret_copy = ret;
	ret = false;
	return ret_copy;
}

void AssetManagerWindow::RenderAddAssetPopup(bool open_condition) {
	if (open_condition) {
		ImGui::OpenPopup("Add asset");
	}

	if (ImGui::BeginPopup("Add asset")) {
		if (ImGui::Selectable("Mesh")) mp_active_add_asset_window = [this]{ if (RenderAddMeshAssetWindow()) mp_active_add_asset_window = nullptr;};
		if (ImGui::Selectable("Texture2D")) mp_active_add_asset_window = [this]{ if (RenderAddTexture2DAssetWindow()) mp_active_add_asset_window = nullptr;};
		if (ImGui::Selectable("Material")) mp_active_add_asset_window = [this]{ if (RenderAddMaterialAssetWindow()) mp_active_add_asset_window = nullptr;};
		if (ImGui::Selectable("Script")) mp_active_add_asset_window = [this]{ if (RenderAddScriptAssetWindow()) mp_active_add_asset_window = nullptr;};
		if (ImGui::Selectable("Prefab")) mp_active_add_asset_window = [this]{ if (RenderAddPrefabAssetWindow()) mp_active_add_asset_window = nullptr;};
		if (ImGui::Selectable("Sound")) mp_active_add_asset_window = [this]{ if (RenderAddSoundAssetWindow()) mp_active_add_asset_window = nullptr;};
		if (ImGui::Selectable("PhysX Material")) mp_active_add_asset_window = [this]{ if (RenderAddPhysxMaterialAssetWindow()) mp_active_add_asset_window = nullptr;};
		if (ImGui::Selectable("Scene")) mp_active_add_asset_window = [this]{ if (RenderAddSceneAssetWindow()) mp_active_add_asset_window = nullptr;};

		ImGui::EndPopup();
	}

	if (mp_active_add_asset_window) mp_active_add_asset_window();
}

bool AssetManagerWindow::RenderBaseAddAssetWindow(const AssetAddDisplaySpec& display_spec, std::string& name, std::string& filepath, const std::string& extension) {
	bool ret = false;
	if (ImGui::Begin("Addasset", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::SeparatorText("Add asset"); ImGui::SameLine(); if (ImGui::Button("X")) {mp_active_add_asset_window = nullptr; return false; };
		ImGui::Text("Name: ");
		ImGui::SameLine();
		ExtraUI::AlphaNumTextInput(name);

		if (display_spec.on_render) display_spec.on_render();

		if (ImGui::Button("Add")) {
			const std::string new_asset_fp = m_current_content_dir + "/" + name + extension;
			if (!CanCreateAsset(new_asset_fp)) {
				ORNG_CORE_ERROR("Cannot create asset with path '{}', invalid path", new_asset_fp);
				ret = false;
			} else {
				filepath = new_asset_fp;
				ret = true;
			}
		}
	}
	ImGui::End();

	return ret;
}

bool AssetManagerWindow::CanCreateAsset(const std::string& asset_filepath) {
	const std::string filename = GetFilename(asset_filepath);
	return !std::filesystem::exists(asset_filepath) && !filename.empty() && std::isalpha(filename[0]);
}

bool AssetManagerWindow::RenderAddMeshAssetWindow() {
	static std::string raw_mesh_filepath = "";

	AssetAddDisplaySpec spec{};
	spec.on_render = [this] {
		ImGui::Text(std::format("Source: {}", raw_mesh_filepath).c_str());
		if (ImGui::Button("Add mesh source file")) {
			wchar_t valid_extensions[MAX_PATH] = L"Mesh Files: *.obj;*.fbx;*.glb;*.gltf\0*.obj;*.fbx;*.glb;*.gltf\0";

			std::function<void(std::string)> success_callback = [this](std::string filepath) {
				raw_mesh_filepath = filepath;
			};

			ExtraUI::ShowFileExplorer("", valid_extensions, success_callback);
		}
	};


	static std::string name = "New asset";
	static std::string new_asset_fp;
	const bool add = RenderBaseAddAssetWindow(spec, name, new_asset_fp, ".omesh");
	if (!add || name.empty() || raw_mesh_filepath.empty()) return false;

	auto* p_asset = new MeshAsset{new_asset_fp};
	p_asset->filepath = m_current_content_dir + "/" + ReplaceFileExtension(GetFilename(raw_mesh_filepath), "") + ".omesh";
	AssetManager::GetSerializer().LoadMeshAsset(p_asset, raw_mesh_filepath);

	new_asset_fp = "";
	name = "New asset";

	return true;
}

bool AssetManagerWindow::RenderAddTexture2DAssetWindow() {
	static std::string raw_tex_filepath;

	AssetAddDisplaySpec spec{};
	spec.on_render = [this] {
		ImGui::Text(std::format("Source: {}", raw_tex_filepath).c_str());
		if (ImGui::Button("Add texture source file")) {
			wchar_t valid_extensions[MAX_PATH] = L"Texture Files: *.png;*.jpg;*.jpeg;*.hdr\0*.png;*.jpg;*.jpeg;*.hdr\0";

			std::function<void(std::string)> success_callback = [this](std::string filepath) {
				raw_tex_filepath = filepath;
			};

			ExtraUI::ShowFileExplorer("", valid_extensions, success_callback);
		}
	};

	static std::string name = "New asset";
	static std::string new_asset_fp = "";
	const bool add = RenderBaseAddAssetWindow(spec, name, new_asset_fp, ".otex");
	if (!add || name.empty() || raw_tex_filepath.empty()) return false;

	m_current_2d_tex_spec.filepath = raw_tex_filepath;
	m_current_2d_tex_spec.generate_mipmaps = true;
	m_current_2d_tex_spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;

	Texture2D* p_new_tex = new Texture2D{new_asset_fp};
	p_new_tex->SetSpec(m_current_2d_tex_spec);
	AssetManager::GetSerializer().LoadTexture2D(AssetManager::AddAsset(p_new_tex));
	p_new_tex->filepath = new_asset_fp;
	new_asset_fp = "";
	name = "New asset";

	return true;
}

bool AssetManagerWindow::RenderAddMaterialAssetWindow() {
	AssetAddDisplaySpec spec{};
	static std::string name = "New asset";
	static std::string new_asset_fp;
	const bool add = RenderBaseAddAssetWindow(spec, name, new_asset_fp, ".omat");
	if (!add || name.empty()) return false;

	auto* p_mat = new Material{new_asset_fp};
	p_mat->name = name;
	AssetManager::AddAsset(p_mat);
	AssetManager::GetSerializer().SerializeAssetToBinaryFile(*p_mat, new_asset_fp);
	new_asset_fp = "";
	name = "New asset";

	return true;
}

bool AssetManagerWindow::RenderAddPrefabAssetWindow() {
	static uint64_t prefab_entity_uuid = 0;

	AssetAddDisplaySpec spec{};
	spec.on_render = [this] {
		ImGui::Text("Drag and drop an entity into the area below");
		ImGui::Button("Drop here!", {100, 200});
		if (prefab_entity_uuid != 0) {
			ImGui::SameLine();
			ImGui::Text(std::format("Prefab entity UUID: {}", prefab_entity_uuid).c_str());
		}

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* p_payload = ImGui::AcceptDragDropPayload("ENTITY"); p_payload && p_payload->DataSize == sizeof(std::vector<uint64_t>)) {
				std::vector<uint64_t>& id_vec = *static_cast<std::vector<uint64_t>*>(p_payload->Data);

				if (!id_vec.empty()) {
					prefab_entity_uuid = id_vec[0];
				}
			}
		}
	};

	static std::string name = "New asset";
	static std::string new_asset_fp;
	const bool add = RenderBaseAddAssetWindow(spec, name, new_asset_fp, ".opfb");
	if (!add || name.empty() || prefab_entity_uuid == 0) return false;

	SceneEntity* p_prefab_ent = mp_scene_context->GetEntity(prefab_entity_uuid);
	if (!p_prefab_ent) return false;

	CreateAndSerializePrefab(*p_prefab_ent, new_asset_fp);

	new_asset_fp = "";
	name = "New asset";
	prefab_entity_uuid = 0;

	return true;
}

bool AssetManagerWindow::RenderAddScriptAssetWindow() {
	AssetAddDisplaySpec spec{};
	static std::string subdirectory_path;
	spec.on_render = [] {
		ImGui::Text("Name must be a valid C++ class name: no whitespace, starts with an alphabetic character, and only alpha-numerical characters");
		ImGui::Separator();
		ImGui::Text("Subdirectory path");
		ImGui::SameLine();
		if (ImGui::IsItemHovered() && ImGui::BeginTooltip()) {
			ImGui::Text(R"(The path, relative to the src and headers folder where your script will be created, e.g if subdirectory path is 'player/movement',
			the script files will be created at res/scripts/src/player/movement/ScriptName.cpp and res/scripts/headers/player/movement/ScriptName.h)");
			ImGui::EndTooltip();
		}
		ImGui::InputText("##subdir", &subdirectory_path);
	};

	static std::string name = "NewScript";
	static std::string _; // unused here
	const bool add = RenderBaseAddAssetWindow(spec, name, _, ".cpp");

	bool valid_name = true;
	StringReplace(name, " ", "");
	std::ranges::for_each(name, [&valid_name](char c) { if (!std::isalnum(c)) valid_name = false; });
	valid_name &= !name.empty() && std::isalpha(name[0]);
	std::ranges::for_each(subdirectory_path, [&valid_name](char c) { if (!std::isalnum(c) && c != '/') valid_name = false; });
	if(!subdirectory_path.empty()) valid_name &= std::isalpha(subdirectory_path[0]);

	std::string script_path_h = "res/scripts/headers/" + subdirectory_path + '/' + name + ".h";
	std::string script_path_cpp = "res/scripts/src/" + subdirectory_path + '/' + name + ".cpp";
	StringReplace(script_path_cpp, "//", "/"); // remove double slashes
	if (!add || !valid_name || FileExists(script_path_h) || FileExists(script_path_cpp)) return false;

	if (!FileExists("res/scripts/headers/" + subdirectory_path)) {
		std::filesystem::create_directories("res/scripts/headers/" + subdirectory_path);
	}

	if (!FileExists("res/scripts/src/" + subdirectory_path)) {
		std::filesystem::create_directories("res/scripts/src/" + subdirectory_path);
	}

	// Update here as certain directories need to be initialized if this is the first script created
	ScriptingEngine::UpdateScriptCmakeProject("res/scripts");

	if (!AssetManager::GetAsset<ScriptAsset>(script_path_cpp)) {
		std::string script_template_h_content = ReadTextFile(ORNG_CORE_MAIN_DIR "/src/scripting/ScriptingTemplate.h");
		std::string script_template_cpp_content = ReadTextFile(ORNG_CORE_MAIN_DIR "/src/scripting/ScriptingTemplate.cpp");
		StringReplace(script_template_h_content, "ScriptClassExample", name);

		const auto uuid = UUID<uint64_t>{};
		StringReplace(script_template_cpp_content, "REPLACE_ME_UUID", std::to_string(uuid()));
		StringReplace(script_template_cpp_content, "ScriptClassExample", name);
		WriteTextFile(script_path_h, script_template_h_content);
		WriteTextFile(script_path_cpp, script_template_cpp_content);
		auto* p_script = new ScriptAsset{script_path_cpp, false};
		p_script->uuid = uuid;
		AssetManager::AddAsset(p_script);
	}

	ScriptingEngine::UpdateScriptCmakeProject("res/scripts");
	name = "NewScript";
	subdirectory_path = "";

	return true;
}

bool AssetManagerWindow::RenderAddPhysxMaterialAssetWindow() {
	AssetAddDisplaySpec spec{};

	static std::string name = "New asset";
	static std::string new_asset_fp;
	const bool add = RenderBaseAddAssetWindow(spec, name, new_asset_fp, ".opmat");
	if (!add || name.empty()) return false;

	auto* p_new = new PhysXMaterialAsset{new_asset_fp};
	p_new->p_material = Physics::GetPhysics()->createMaterial(0.75, 0.75, 0.6);
	AssetManager::AddAsset(p_new);

	return true;
}

bool AssetManagerWindow::RenderAddSoundAssetWindow() {
	static std::string raw_sound_filepath;

	AssetAddDisplaySpec spec{};
	spec.on_render = [this] {
		ImGui::Text(std::format("Source: {}", raw_sound_filepath).c_str());
		if (ImGui::Button("Add audio source file")) {
			wchar_t valid_extensions[MAX_PATH] = L"Audio Files: *.mp3;*.wav;\0*.mp3;*.wav;\0";

			std::function<void(std::string)> success_callback = [this](std::string filepath) {
				raw_sound_filepath = filepath;
			};

			ExtraUI::ShowFileExplorer("", valid_extensions, success_callback);
		}
	};

	static std::string name = "New asset";
	static std::string new_asset_fp = "";
	const bool add = RenderBaseAddAssetWindow(spec, name, new_asset_fp, ".osound");
	if (!add || name.empty() || raw_sound_filepath.empty()) return false;

	auto* p_sound = new SoundAsset{new_asset_fp};
	p_sound->source_filepath = raw_sound_filepath;
	AssetManager::AddAsset(p_sound);
	p_sound->CreateSoundFromFile();

	name = "New asset";
	new_asset_fp = "";

	return true;
}

bool AssetManagerWindow::RenderAddSceneAssetWindow() {
	AssetAddDisplaySpec spec{};

	static std::string name = "New scene";
	static std::string new_asset_fp;
	const bool add = RenderBaseAddAssetWindow(spec, name, new_asset_fp, ".oscene");
	if (!add || name.empty()) return false;

	auto* p_new = new SceneAsset{new_asset_fp};
	std::string yml = ORNG_BASE_SCENE_YAML;
	StringReplace(yml, std::to_string(ORNG_BASE_SCENE_UUID), std::to_string(p_new->uuid()));
	p_new->node = YAML::Load(yml);
	AssetManager::AddAsset(p_new);

	return true;
}

void AssetManagerWindow::RenderMainAssetWindow() {
	const int window_width = Window::GetWidth() - 650;
	column_count = glm::max<int>(window_width / (image_button_size.x + 40), 1);

	if (ImGui::Button(ICON_FA_ARROW_LEFT_LONG)) {
		m_current_content_dir = m_current_content_dir.substr(0, m_current_content_dir.rfind('/'));
	}

	ImGui::SameLine();
	RenderAddAssetPopup(ImGui::Button("+"));

	auto path_directories = SplitString(m_current_content_dir, '/');
	for (size_t i = 0; i < path_directories.size(); i++) {
		ImGui::SameLine(0.f, i == 0 ? -1 : 0.f);
		ImGui::Text((path_directories[i] + (i == path_directories.size() - 1 ? "" : " >> ")).c_str());

		if (ImGui::IsItemClicked()) {
			m_current_content_dir = "";
			for (size_t j = 0; j <= i; j++) {
				m_current_content_dir += path_directories[j] + "/";
			}
			break;
		}
	}

	if (ImGui::BeginTable(m_current_content_dir.c_str(), column_count))
	{
		for (const auto& entry : std::filesystem::directory_iterator{m_current_content_dir}) {
			if (!entry.is_directory()) continue;

			ImGui::TableNextColumn();

			if (RenderDirectory(entry.path(), m_current_content_dir)) {
				break;
			}
		}

		for (auto& [uuid, p_asset] : AssetManager::Get().m_assets) {
			if (uuid < static_cast<uint64_t>(BaseAssetIDs::NUM_BASE_ASSETS)) continue;

			auto parent_path = std::filesystem::path{p_asset->filepath}.parent_path();
			auto current_dir_fs = std::filesystem::path{m_current_content_dir};
			auto comp = parent_path.compare(current_dir_fs);
			if (comp != 0) continue;
			ImGui::TableNextColumn();
			RenderAsset(p_asset);
		}
		ImGui::EndTable();
	}

	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1) && !ImGui::IsPopupOpen("Directory settings", ImGuiPopupFlags_AnyPopup)) {
		ImGui::OpenPopup("Content options");
	}
	if (ImGui::BeginPopup("Content options")) {
		static std::string dir_name;
		ImGui::InputText("##dirname", &dir_name);
		if (ImGui::Selectable("Create directory")) {
			std::filesystem::create_directories(m_current_content_dir + "/" +  dir_name);
			dir_name = "";
		}
		ImGui::EndPopup();
	}

}



void AssetManagerWindow::OnProjectEvent(const Events::AssetEvent& t_event) {
	switch (t_event.event_type) {
		case Events::AssetEventType::MESH_LOADED: {
			auto* p_data = reinterpret_cast<std::pair<AssetSerializer::MeshAssets*, MeshLoadResult*>*>(t_event.data_payload);
			auto& [assets, load_result] = *p_data;

			m_meshes_to_gen_previews.push_back(assets->p_mesh);

			if (assets->p_mesh->uuid() == static_cast<uint64_t>(BaseAssetIDs::SPHERE_MESH) || assets->p_mesh->uuid() == static_cast<uint64_t>(BaseAssetIDs::CUBE_MESH))
				return;

			const std::string& filepath = assets->p_mesh->filepath;
			if (!FileExists(filepath) && filepath.substr(0, filepath.size() - 4).find(".omesh") == std::string::npos) {
				// Gen binary file if none exists
				AssetManager::GetSerializer().SerializeAssetToBinaryFile(*assets->p_mesh, filepath);

				// Serialize materials
				const std::string filepath_no_extension = ReplaceFileExtension(filepath, "");
				for (size_t i = 0; i < assets->materials.size(); i++) {
					auto* p_mat = assets->materials[i];
					p_mat->filepath = std::format("{}_mat_{}.omat", filepath_no_extension, p_mat->name.empty() ? std::to_string(i) : StripNonAlphaNumeric(p_mat->name));
					AssetManager::GetSerializer().SerializeAssetToBinaryFile(*p_mat, p_mat->filepath);
				}

				// Serialize textures
				for (size_t i = 0; i < assets->textures.size(); i++) {
					auto* p_tex = assets->textures[i];
					p_tex->filepath = std::format("{}_tex_{}.otex", filepath_no_extension, p_tex->GetName().empty() ? std::to_string(i) : StripNonAlphaNumeric(p_tex->GetName()));
					AssetManager::GetSerializer().SerializeAssetToBinaryFile(*p_tex, p_tex->filepath);
				}
			}
			break;
		}
		case Events::AssetEventType::MATERIAL_LOADED:
		{
			auto* p_material = reinterpret_cast<Material*>(t_event.data_payload);
			m_materials_to_gen_previews.push_back(p_material);
			break;
		}
	}
}



void AssetManagerWindow::CreateMeshPreview(MeshAsset* p_asset) {
	auto p_tex = std::make_shared<Texture2D>(p_asset->filepath.substr(p_asset->filepath.find_last_of("/") + 1) + " - Mesh preview");
	p_tex->SetSpec(m_asset_preview_spec);
	m_mesh_preview_textures[p_asset] = p_tex;

	// Scale mesh so it always fits in camera frustum
	glm::vec3 extents = p_asset->GetAABB().extents;
	float largest_extent = glm::max(glm::max(extents.x, extents.y), extents.z);
	glm::vec3 scale_factor = glm::vec3(1.0, 1.0, 1.0) / largest_extent;

	mp_preview_scene->GetSystem<SceneUBOSystem>().OnUpdate();

	mp_preview_scene->GetEntity("Sphere")->GetComponent<TransformComponent>()->SetScale(scale_factor);
	mp_preview_scene->GetEntity("Sphere")->GetComponent<MeshComponent>()->SetMeshAsset(p_asset);
	mp_preview_scene->GetSystem<MeshInstancingSystem>().OnUpdate();

	m_preview_render_graph.SetData("OutCol", p_tex.get());
	m_preview_render_graph.SetData("BloomInCol", p_tex.get());
	m_preview_render_graph.Execute();

	glGenerateTextureMipmap(p_tex->GetTextureHandle());
}

void AssetManagerWindow::CreateMaterialPreview(const Material* p_material) {
	auto p_tex = std::make_shared<Texture2D>(p_material->name + " - Material preview");
	p_tex->SetSpec(m_asset_preview_spec);

	m_material_preview_textures[p_material] = p_tex;

	auto* p_mesh = mp_preview_scene->GetEntity("Sphere")->GetComponent<MeshComponent>();
	if (auto* p_sphere_mesh = AssetManager::GetAsset<MeshAsset>(static_cast<uint64_t>(BaseAssetIDs::SPHERE_MESH)); p_mesh->GetMeshData() != p_sphere_mesh)
		p_mesh->SetMeshAsset(p_sphere_mesh);

	mp_preview_scene->GetSystem<SceneUBOSystem>().OnUpdate();
	auto& mesh_sys = mp_preview_scene->GetSystem<MeshInstancingSystem>();
	mp_preview_scene->GetEntity("Sphere")->GetComponent<TransformComponent>()->SetScale(1, 1, 1);
	p_mesh->SetMaterialID(0, p_material);

	mesh_sys.OnUpdate();

	m_preview_render_graph.SetData("OutCol", p_tex.get());
	m_preview_render_graph.SetData("BloomInCol", p_tex.get());
	m_preview_render_graph.Execute();

	glGenerateTextureMipmap(p_tex->GetTextureHandle());
}



void AssetManagerWindow::RenderPhysXMaterialEditor() {
	if (!mp_selected_physx_material)
		return;

	static float df = 1.f;
	static float sf = 1.f;
	static float r = 1.f;

	df = mp_selected_physx_material->p_material->getDynamicFriction();
	sf = mp_selected_physx_material->p_material->getStaticFriction();
	r = mp_selected_physx_material->p_material->getRestitution();

	ImGui::SetNextWindowSize({ 600, 300 });

	if (ImGui::Begin("PhysX material editor")) {
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(1)) {
			// Deselect, close window
			mp_selected_physx_material = nullptr;
			ImGui::End();
			return;
		}

		ImGui::PushItemWidth(300.0);
		float avail = ImGui::GetContentRegionAvail().x;

		ImGui::Text("Name: "); ImGui::SameLine(avail - 300.0);
		ExtraUI::AlphaNumTextInput(mp_selected_physx_material->name);

		ImGui::Text("Dynamic friction"); ImGui::SameLine(avail - 300.0);
		if (ImGui::DragFloat("##df", &df)) {
			mp_selected_physx_material->p_material->setDynamicFriction(df);
		}

		ImGui::Text("Static friction"); ImGui::SameLine(avail - 300.0);
		if (ImGui::DragFloat("##sf", &sf)) {
			mp_selected_physx_material->p_material->setStaticFriction(sf);
		}

		ImGui::Text("Restitution"); ImGui::SameLine(avail - 300.0);
		if (ImGui::SliderFloat("##r", &r, 0.f, 1.f)) {
			mp_selected_physx_material->p_material->setRestitution(r);
		}

		ImGui::PopItemWidth();
	}
	ImGui::End();
}

bool AssetManagerWindow::RenderMaterialEditorSection() {
	bool ret = false;

	if (ImGui::Begin("Material editor")) {
		MaterialFlags flags = mp_selected_material->GetFlags();

		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Right)) {
			// Hide this tree node
			mp_selected_material = nullptr;
			goto window_end;
		}


		ImGui::Text("Name: ");
		ImGui::SameLine();
		ImGui::InputText("##name input", &mp_selected_material->name);
		ImGui::Spacing();

		ret |= RenderMaterialTexture("Base", mp_selected_material->base_colour_texture);
		ret |= RenderMaterialTexture("Normal", mp_selected_material->normal_map_texture);
		ret |= RenderMaterialTexture("Roughness", mp_selected_material->roughness_texture);
		ret |= RenderMaterialTexture("Metallic", mp_selected_material->metallic_texture);
		ret |= RenderMaterialTexture("AO", mp_selected_material->ao_texture);
		ret |= RenderMaterialTexture("Displacement", mp_selected_material->displacement_texture);
		ret |= RenderMaterialTexture("Emissive", mp_selected_material->emissive_texture);

		ImGui::Text("Colors");
		ImGui::Spacing();
		ret |= ExtraUI::ShowVec4Editor("Base color", mp_selected_material->base_colour);
		if (mp_selected_material->base_colour.w < 0.0 || mp_selected_material->base_colour.w > 1.0)
			mp_selected_material->base_colour.w = 1.0;

		if (!mp_selected_material->roughness_texture)
			ret |= ImGui::SliderFloat("Roughness", &mp_selected_material->roughness, 0.f, 1.f);

		if (!mp_selected_material->metallic_texture)
			ret |= ImGui::SliderFloat("Metallic", &mp_selected_material->metallic, 0.f, 1.f);

		if (!mp_selected_material->ao_texture)
			ret |= ImGui::SliderFloat("AO", &mp_selected_material->ao, 0.f, 1.f);

		ret |= ImGui::SliderFloat("Alpha cutoff", &mp_selected_material->alpha_cutoff, 0.f, 1.f);

		static bool emissive = false;
		emissive = (flags & ORNG_MatFlags_EMISSIVE);
		if (ImGui::Checkbox("Emissive", &emissive)) {
			mp_selected_material->FlipFlags(ORNG_MatFlags_EMISSIVE);
			ret = true;
		}

		static bool disable_face_culling = false;
		disable_face_culling = (flags & ORNG_MatFlags_DISABLE_BACKFACE_CULL);
		if (ImGui::Checkbox("Disable backface culling", &disable_face_culling)) {
			mp_selected_material->FlipFlags(ORNG_MatFlags_DISABLE_BACKFACE_CULL);
			ret = true;
		}

		if ((flags & ORNG_MatFlags_EMISSIVE) || mp_selected_material->emissive_texture)
			ret |= ImGui::SliderFloat("Emissive strength", &mp_selected_material->emissive_strength, -10.f, 10.f);

		int num_parallax_layers = mp_selected_material->parallax_layers;
		if (mp_selected_material->displacement_texture) {
			ImGui::SeparatorText("Displacement");

			static bool tessellated = false;
			tessellated = (flags & ORNG_MatFlags_TESSELLATED);
			static bool parallax = false;
			parallax = (flags & ORNG_MatFlags_PARALLAX_MAP);

			if (ImGui::Checkbox("Tessellated", &tessellated)) {
				mp_selected_material->FlipFlags(ORNG_MatFlags_TESSELLATED);
				ret = true;
			}

			if (ImGui::Checkbox("Parallax mapping", &parallax)) {
				mp_selected_material->FlipFlags(ORNG_MatFlags_PARALLAX_MAP);
				ret = true;
			}

			if (parallax) {
				ret |= ImGui::InputInt("Parallax layers", &num_parallax_layers);

				if (num_parallax_layers >= 0)
					mp_selected_material->parallax_layers = num_parallax_layers;

				ret |= ImGui::InputFloat("Parallax scale", &mp_selected_material->displacement_scale);
			}
		}

		ret |= ExtraUI::ShowVec2Editor("Tile scale", mp_selected_material->tile_scale);

		static bool is_spritesheet = false;
		is_spritesheet = (flags & ORNG_MatFlags_IS_SPRITESHEET);

		if (ImGui::Checkbox("Spritesheet", &is_spritesheet)) {
			mp_selected_material->FlipFlags(ORNG_MatFlags_IS_SPRITESHEET);
			ret = true;
		}

		if (flags & ORNG_MatFlags_IS_SPRITESHEET) {
			ret |= ExtraUI::InputUint("Rows", mp_selected_material->spritesheet_data.num_rows);
			ret |= ExtraUI::InputUint("Cols", mp_selected_material->spritesheet_data.num_cols);
		}


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

			std::string mipmapped = std::format("Mipmaps: {}", mp_selected_texture->GetSpec().generate_mipmaps ? "true" : "false");
			ImGui::Text(mipmapped.c_str());

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

void AssetManagerWindow::OnRequestDeleteAsset(Asset* p_asset, const std::string& confirmation_text, const std::function<void()>& callback) {
	PushConfirmationWindow("Delete asset? " + confirmation_text, [=] {
		if (callback)
			callback();

		// Cleanup binary file
		FileDelete(p_asset->filepath);

		m_asset_deletion_queue.push_back(p_asset->uuid());
		});
};
