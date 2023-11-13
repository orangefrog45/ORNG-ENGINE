#include "pch/pch.h"
#include "assets/AssetManager.h"
#include "core/CodedAssets.h"
#include "events/EventManager.h"
#include "core/Window.h"
#include "assimp/scene.h"
#include "rendering/Textures.h"
#include "rendering/MeshAsset.h"
#include "audio/AudioEngine.h"
#include <fmod.hpp>
#include <fmod_errors.h>
#include "core/GLStateManager.h"
#include "scene/SceneSerializer.h"


// For glfwmakecontextcurrent
#include <GLFW/glfw3.h>




namespace ORNG {
	void AssetManager::I_Init() {
		glfwWindowHint(GLFW_VISIBLE, 0);
		mp_loading_context = glfwCreateWindow(100, 100, "ASSET_LOADING_CONTEXT", nullptr, Window::GetGLFWwindow());
		InitBaseAssets();

		// Each frame, check if any meshes have finished loading vertex data and load them into GPU if they have
		m_update_listener.OnEvent = [this](const Events::EngineCoreEvent& t_event) {
			if (t_event.event_type == Events::EngineCoreEvent::EventType::ENGINE_UPDATE) {
				for (int i = 0; i < m_mesh_loading_queue.size(); i++) {
					[[unlikely]] if (m_mesh_loading_queue[i].wait_for(std::chrono::nanoseconds(1)) == std::future_status::ready) {
						auto* p_mesh = m_mesh_loading_queue[i].get();

						m_mesh_loading_queue.erase(m_mesh_loading_queue.begin() + i);
						i--;

						LoadMeshAssetIntoGL(p_mesh);

						Events::AssetEvent e_event;
						e_event.event_type = Events::AssetEventType::MESH_LOADED;
						e_event.data_payload = reinterpret_cast<uint8_t*>(p_mesh);
						Events::EventManager::DispatchEvent(e_event);
					}
				}
			}
			};

		Events::EventManager::RegisterListener(m_update_listener);
	}



	void AssetManager::IStallUntilMeshesLoaded() {
		while (!m_mesh_loading_queue.empty()) {
			for (int i = 0; i < m_mesh_loading_queue.size(); i++) {
				[[unlikely]] if (m_mesh_loading_queue[i].wait_for(std::chrono::nanoseconds(1)) == std::future_status::ready) {
					auto* p_mesh_asset = m_mesh_loading_queue[i].get();

					m_mesh_loading_queue.erase(m_mesh_loading_queue.begin() + i);
					i--;

					LoadMeshAssetIntoGL(p_mesh_asset);
					for (auto& future : m_texture_futures) {
						future.get();
					}
					m_texture_futures.clear();
					DispatchAssetEvent(Events::AssetEventType::MESH_LOADED, reinterpret_cast<uint8_t*>(p_mesh_asset));
				}
			}
		}
	}

	void AssetManager::DispatchAssetEvent(Events::AssetEventType type, uint8_t* data_payload) {
		Events::AssetEvent e_event;
		e_event.event_type = type;
		e_event.data_payload = data_payload;
		Events::EventManager::DispatchEvent(e_event);
	}

	void AssetManager::IClearAll() {
		auto it = m_assets.begin();
		while (it != m_assets.end()) {
			if (it->first <= ORNG_BASE_SPHERE_ID) {
				it++;
				continue;
			}

			HandleAssetDeletion(it->second);
			delete it->second;
			it = m_assets.erase(it);
		}
	}

	void AssetManager::LoadTexture2D(Texture2D* p_tex) {
		static std::mutex m;
		Get().m_texture_futures.push_back(std::async(std::launch::async, [p_tex] {
			m.lock();
			glfwMakeContextCurrent(Get().mp_loading_context);
			p_tex->LoadFromFile();
			DispatchAssetEvent(Events::AssetEventType::TEXTURE_LOADED, reinterpret_cast<uint8_t*>(p_tex));
			glfwMakeContextCurrent(nullptr);
			m.unlock();
			}));
	}



	void AssetManager::OnTextureDelete(Texture2D* p_tex) {
		// If any materials use this texture, remove it from them
		for (auto& [key, p_asset] : Get().m_assets) {
			auto* p_material = dynamic_cast<Material*>(p_asset);
			if (!p_material)
				continue;

			p_material->base_color_texture = p_material->base_color_texture == p_tex ? nullptr : p_material->base_color_texture;
			p_material->normal_map_texture = p_material->normal_map_texture == p_tex ? nullptr : p_material->normal_map_texture;
			p_material->emissive_texture = p_material->emissive_texture == p_tex ? nullptr : p_material->emissive_texture;
			p_material->displacement_texture = p_material->displacement_texture == p_tex ? nullptr : p_material->displacement_texture;
			p_material->metallic_texture = p_material->metallic_texture == p_tex ? nullptr : p_material->metallic_texture;
			p_material->roughness_texture = p_material->roughness_texture == p_tex ? nullptr : p_material->roughness_texture;
		}
	}





	void AssetManager::LoadMeshAssetIntoGL(MeshAsset* asset) {
		if (asset->m_is_loaded) {
			ORNG_CORE_ERROR("Mesh '{0}' is already loaded", asset->filepath);
			return;
		}

		GL_StateManager::BindVAO(asset->GetVAO().GetHandle());

		// Get directory used for finding material textures
		std::string::size_type slash_index = asset->filepath.find_last_of("\\");
		std::string dir;

		if (slash_index == std::string::npos) {
			dir = ".";
		}
		else if (slash_index == 0) {
			dir = "\\";
		}
		else {
			dir = asset->filepath.substr(0, slash_index);
		}

		// p_scene will be nullptr if the mesh was loaded from a binary file, then default materials will be provided
		if (asset->p_scene) {
			for (unsigned int i = 0; i < asset->p_scene->mNumMaterials; i++) {
				asset->num_materials++;
				const aiMaterial* p_material = asset->p_scene->mMaterials[i];
				Material* p_new_material = Get().AddAsset(new Material());

				// Load material textures
				p_new_material->base_color_texture = CreateMeshAssetTexture(dir, aiTextureType_BASE_COLOR, p_material);
				p_new_material->normal_map_texture = CreateMeshAssetTexture(dir, aiTextureType_NORMALS, p_material);
				p_new_material->roughness_texture = CreateMeshAssetTexture(dir, aiTextureType_DIFFUSE_ROUGHNESS, p_material);
				p_new_material->metallic_texture = CreateMeshAssetTexture(dir, aiTextureType_METALNESS, p_material);
				p_new_material->ao_texture = CreateMeshAssetTexture(dir, aiTextureType_AMBIENT_OCCLUSION, p_material);

				// Load material properties
				aiColor3D base_color(0.0f, 0.0f, 0.0f);
				if (p_material->Get(AI_MATKEY_BASE_COLOR, base_color) == aiReturn_SUCCESS) {
					p_new_material->base_color.r = base_color.r;
					p_new_material->base_color.g = base_color.g;
					p_new_material->base_color.b = base_color.b;
				}

				float roughness;
				float metallic;

				if (p_material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == aiReturn_SUCCESS)
					p_new_material->roughness = roughness;

				if (p_material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == aiReturn_SUCCESS)
					p_new_material->metallic = metallic;

				// Check if the material has had any properties actually set - if not then use the default material instead of creating a new one.
				if (!p_new_material->base_color_texture && !p_new_material->normal_map_texture && !p_new_material->roughness_texture
					&& !p_new_material->metallic_texture && !p_new_material->ao_texture && p_new_material->roughness == 0.2f && p_new_material->metallic == 0.0f) {
					DeleteAsset(p_new_material);
				}
			}
		}

		asset->PopulateBuffers();
		asset->m_is_loaded = true;
		DispatchAssetEvent(Events::AssetEventType::MESH_LOADED, reinterpret_cast<uint8_t*>(asset));
	}

	void AssetManager::LoadMeshAsset(MeshAsset* p_asset) {
		Get().m_mesh_loading_queue.emplace_back(std::async(std::launch::async, [p_asset] {
			p_asset->LoadMeshData();
			return p_asset;
			}));
	};





	Texture2D* AssetManager::CreateMeshAssetTexture(const std::string& dir, const aiTextureType& type, const aiMaterial* p_material) {
		Texture2D* p_tex = nullptr;


		if (p_material->GetTextureCount(type) > 0) {
			aiString path;

			if (p_material->GetTexture(type, 0, &path, NULL, NULL, NULL, NULL, NULL) == aiReturn_SUCCESS) {
				std::string p(path.data);
				std::string full_path;

				if (p.starts_with(".\\"))
					p = p.substr(2, p.size() - 2);

				full_path = dir + "\\" + p;

				Texture2DSpec base_spec;
				base_spec.generate_mipmaps = true;
				base_spec.mag_filter = GL_LINEAR;
				base_spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
				base_spec.filepath = full_path;
				base_spec.srgb_space = type == aiTextureType_BASE_COLOR ? true : false;

				p_tex = new Texture2D(full_path);
				p_tex->SetSpec(base_spec);
				p_tex = AddAsset(p_tex);
				Get().LoadTexture2D(p_tex);
			}
		}

		return p_tex;
	}


	void AssetManager::HandleAssetAddition(Asset* p_asset) {
		if (auto* p_material = dynamic_cast<Material*>(p_asset)) {
			DispatchAssetEvent(Events::AssetEventType::MATERIAL_LOADED, reinterpret_cast<uint8_t*>(p_material));
		}
	}

	void AssetManager::HandleAssetDeletion(Asset* p_asset) {
		if (auto* p_tex = dynamic_cast<Texture2D*>(p_asset)) {
			OnTextureDelete(p_tex);
			DispatchAssetEvent(Events::AssetEventType::TEXTURE_DELETED, reinterpret_cast<uint8_t*>(p_tex));
		}
		if (auto* p_mesh = dynamic_cast<MeshAsset*>(p_asset)) {
			DispatchAssetEvent(Events::AssetEventType::MESH_DELETED, reinterpret_cast<uint8_t*>(p_mesh));
		}
		if (auto* p_material = dynamic_cast<Material*>(p_asset)) {
			DispatchAssetEvent(Events::AssetEventType::MATERIAL_DELETED, reinterpret_cast<uint8_t*>(p_material));
		}
		else if (auto* p_script = dynamic_cast<ScriptAsset*>(p_asset)) {
			DispatchAssetEvent(Events::AssetEventType::SCRIPT_DELETED, reinterpret_cast<uint8_t*>(p_script));
			ScriptingEngine::UnloadScriptDLL(p_script->symbols.script_path);
		}
	}


	void AssetManager::LoadAssetsFromProjectPath(const std::string& project_dir, bool precompiled_scripts) {
		std::string texture_folder = project_dir + "\\res\\textures\\";
		std::string mesh_folder = project_dir + "\\res\\meshes\\";
		std::string audio_folder = project_dir + "\\res\\audio\\";
		std::string material_folder = project_dir + "\\res\\materials\\";
		std::string prefab_folder = project_dir + "\\res\\prefabs\\";
		std::string script_folder = project_dir + "\\res\\scripts\\";

		Texture2DSpec default_spec;
		default_spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
		default_spec.mag_filter = GL_LINEAR;
		default_spec.generate_mipmaps = true;
		default_spec.storage_type = GL_UNSIGNED_BYTE;



		for (const auto& entry : std::filesystem::recursive_directory_iterator(mesh_folder)) {
			if (entry.is_directory() || entry.path().extension() != ".bin")
				continue;

			std::string str_path = entry.path().string();
			std::string rel_path = ".\\" + str_path.substr(str_path.rfind("res\\meshes"));
			auto* p_mesh = new MeshAsset(rel_path);
			DeserializeAssetBinary(rel_path, *p_mesh);
			AddAsset(p_mesh);
			LoadMeshAssetIntoGL(p_mesh);
		}


		for (const auto& entry : std::filesystem::recursive_directory_iterator(texture_folder)) {
			std::string path = entry.path().string();
			bool is_tex_loaded = AssetManager::GetAsset<Texture2D>(path) ? true : false;

			if (entry.is_directory() || path.find("diffuse_prefilter") != std::string::npos || is_tex_loaded || entry.path().extension() != ".otex") // Skip serialized diffuse prefilter
				continue;

			default_spec.filepath = path.substr(path.rfind("\\res\\") + 1);
			auto* p_tex = new Texture2D(default_spec.filepath);
			DeserializeAssetBinary(path, *p_tex);
			AddAsset(p_tex);
			LoadTexture2D(p_tex);
		}

		for (const auto& entry : std::filesystem::recursive_directory_iterator(audio_folder)) {
			auto path = entry.path();
			if (entry.is_directory() || !(path.extension() == ".osound"))
				continue;
			else {
				std::string rel_path = ".\\" + entry.path().string().substr(path.string().rfind("res\\audio"));
				auto* p_sound = new SoundAsset(entry.path().string());
				DeserializeAssetBinary(entry.path().string(), *p_sound);
				p_sound->CreateSound();
				AddAsset(p_sound);
			}
		}

		for (const auto& entry : std::filesystem::recursive_directory_iterator(material_folder)) {
			auto path = entry.path();
			if (entry.is_directory() || path.extension() != ".omat")
				continue;
			else {
				std::string rel_path = ".\\" + entry.path().string().substr(path.string().rfind("res\\materials"));
				auto* p_mat = new Material(rel_path);
				DeserializeAssetBinary(rel_path, *p_mat);
				AddAsset(p_mat);
			}
		}

		for (const auto& entry : std::filesystem::recursive_directory_iterator(prefab_folder)) {
			auto path = entry.path();
			if (entry.is_directory() || path.extension() != ".opfb")
				continue;
			else {
				std::string rel_path = ".\\" + path.string().substr(path.string().rfind("res\\prefabs"));
				auto* p_prefab = new Prefab(rel_path);
				DeserializeAssetBinary(rel_path, *p_prefab);
				AddAsset(p_prefab);
			}
		}

		for (const auto& entry : std::filesystem::recursive_directory_iterator(script_folder)) {
			auto path = entry.path();
			if (entry.is_directory() || (!precompiled_scripts && path.extension() != ".cpp") || (precompiled_scripts && path.extension() != ".dll") || path.string().find("scripts\\includes") != std::string::npos)
				continue;
			else {
				std::string path_string = path.string();
				if (precompiled_scripts) {
					std::string dll_path = ".\\" + path_string.substr(path_string.rfind("res\\scripts"));
					auto first = dll_path.rfind("\\");
					std::string rel_path = ".\\res\\scripts" + dll_path.substr(first, dll_path.rfind(".") - first) + ".cpp";
					ScriptSymbols symbols = ScriptingEngine::LoadScriptDll(dll_path, rel_path);

					AddAsset(new ScriptAsset(symbols));
				}
				else {
					std::string rel_path = ".\\" + path_string.substr(path_string.rfind("res\\scripts"));
					ScriptSymbols symbols = ScriptingEngine::GetSymbolsFromScriptCpp(rel_path, precompiled_scripts);
					AddAsset(new ScriptAsset(symbols));
				}
			}
		}
	}


	void AssetManager::SerializeAssets() {
		for (const auto& [uuid, p_asset] : Get().m_assets) {
			auto* p_tex_asset = dynamic_cast<Texture2D*>(p_asset);
			if (!p_tex_asset)
				continue;
			SceneSerializer::SerializeBinary(".\\res\\textures\\" + p_tex_asset->filepath.substr(p_tex_asset->filepath.rfind("\\") + 1) + ".otex", *p_tex_asset);
		}


		for (const auto& [uuid, p_asset] : Get().m_assets) {
			auto* p_material = dynamic_cast<Material*>(p_asset);
			if (!p_material || p_material->uuid() == ORNG_BASE_MATERIAL_ID)
				continue;

			SceneSerializer::SerializeBinary(".\\res\\materials\\" + std::format("{}", p_material->uuid()) + ".omat", *p_material);
		}

		for (auto* p_sound : Get().GetView<SoundAsset>()) {
			if (p_sound->uuid() == ORNG_BASE_SOUND_ID)
				continue;

			std::string fn = p_sound->filepath.substr(p_sound->filepath.rfind("\\") + 1);
			fn = fn.substr(0, fn.rfind(".osound"));
			SceneSerializer::SerializeBinary(".\\res\\audio\\" + fn + ".osound", *p_sound);
		}
	}


	void AssetManager::InitBaseAssets() {
		InitBaseCube();
		InitBaseTexture();
		InitBaseSphere();
		mp_replacement_material = std::make_unique<Material>((uint64_t)ORNG_BASE_MATERIAL_ID);
		auto symbols = ScriptSymbols("");
		mp_base_script = std::make_unique<ScriptAsset>(symbols);
		mp_base_sound = std::make_unique<SoundAsset>(ORNG_CORE_MAIN_DIR "\\res\\audio\\mouse-click.mp3");
		mp_base_cube->uuid = UUID(ORNG_BASE_MESH_ID);
		mp_base_tex->uuid = UUID(ORNG_BASE_TEX_ID);
		mp_replacement_material->uuid = UUID(ORNG_BASE_MATERIAL_ID);
		mp_base_sound->uuid = UUID(ORNG_BASE_SOUND_ID);
		mp_base_script->uuid = UUID(ORNG_BASE_SCRIPT_ID);
		AddAsset(&*mp_base_cube);
		AddAsset(&*mp_base_tex);
		AddAsset(&*mp_replacement_material);
		AddAsset(&*mp_base_sound);
		AddAsset(&*mp_base_script);
		AddAsset(&*mp_base_sphere);
		mp_base_sound->source_filepath = mp_base_sound->filepath;
		mp_base_sound->CreateSound();
	}



	void AssetManager::InitBaseSphere() {
		mp_base_sphere = std::make_unique<MeshAsset>("res/meshes/Sphere.obj");
		DeserializeAssetBinary("res/meshes/Sphere.obj.bin", *mp_base_sphere);
		mp_base_sphere->PopulateBuffers();
		mp_base_sphere->m_is_loaded = true;
		DispatchAssetEvent(Events::AssetEventType::MESH_LOADED, reinterpret_cast<uint8_t*>(&*mp_base_sphere));
	}


	void AssetManager::InitBaseCube() {
		mp_base_cube = std::make_unique<MeshAsset>("");
		mp_base_cube->m_vao.vertex_data.positions = {
			0.5, 0.5, -0.5,
			-0.5, 0.5, -0.5,
			-0.5, 0.5, 0.5,
			0.5, 0.5, 0.5,
			0.5, -0.5, 0.5,
			0.5, 0.5, 0.5,
			-0.5, 0.5, 0.5,
			-0.5, -0.5, 0.5,
			-0.5, -0.5, 0.5,
			-0.5, 0.5, 0.5,
			-0.5, 0.5, -0.5,
			-0.5, -0.5, -0.5,
			-0.5, -0.5, -0.5,
			0.5, -0.5, -0.5,
			0.5, -0.5, 0.5,
			-0.5, -0.5, 0.5,
			0.5, -0.5, -0.5,
			0.5, 0.5, -0.5,
			0.5, 0.5, 0.5,
			0.5, -0.5, 0.5,
			-0.5, -0.5, -0.5,
			-0.5, 0.5, -0.5,
			0.5, 0.5, -0.5,
			0.5, -0.5, -0.5,
		};


		mp_base_cube->m_vao.vertex_data.normals = {
			-0, 1, -0,
			-0, 1, -0,
			-0, 1, -0,
			-0, 1, -0,
			-0, -0, 1,
			-0, -0, 1,
			-0, -0, 1,
			-0, -0, 1,
			-1, -0, -0,
			-1, -0, -0,
			-1, -0, -0,
			-1, -0, -0,
			-0, -1, -0,
			-0, -1, -0,
			-0, -1, -0,
			-0, -1, -0,
			1, -0, -0,
			1, -0, -0,
			1, -0, -0,
			1, -0, -0,
			-0, -0, -1,
			-0, -0, -1,
			-0, -0, -1,
			-0, -0, -1,
		};

		mp_base_cube->m_vao.vertex_data.tangents = {
			-1, 0, 0,
			-1, 0, 0,
			-1, 0, 0,
			-1, 0, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			1, 0, 0,
			1, 0, 0,
			1, 0, 0,
			1, 0, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
		};

		mp_base_cube->m_vao.vertex_data.tex_coords = {
			1.0, 0.0, // Vertex 0
			1.0, 1.0, // Vertex 1
			0.0, 1.0, // Vertex 2
			0.0, 0.0, // Vertex 3

			1.0, 0.0, // Vertex 4
			1.0, 1.0, // Vertex 5
			0.0, 1.0, // Vertex 6
			0.0, 0.0, // Vertex 7

			1.0, 0.0, // Vertex 8
			1.0, 1.0, // Vertex 9
			0.0, 1.0, // Vertex 10
			0.0, 0.0, // Vertex 11

			1.0, 0.0, // Vertex 12
			1.0, 1.0, // Vertex 13
			0.0, 1.0, // Vertex 14
			0.0, 0.0, // Vertex 15

			1.0, 0.0, // Vertex 16
			1.0, 1.0, // Vertex 17
			0.0, 1.0, // Vertex 18
			0.0, 0.0, // Vertex 19

			1.0, 0.0, // Vertex 20
			1.0, 1.0, // Vertex 21
			0.0, 1.0, // Vertex 22
			0.0, 0.0  // Vertex 23
		};


		mp_base_cube->m_vao.vertex_data.indices = {
			// Indices for each face (two triangles per face)
			0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7, 8, 9, 10, 8, 10, 11, 12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23
		};

		mp_base_cube->uuid = UUID(ORNG_BASE_MESH_ID);
		mp_base_cube->m_vao.FillBuffers();
		mp_base_cube->m_aabb.max = { 0.5, 0.5, 0.5 };
		mp_base_cube->m_aabb.min = { -0.5, -0.5, -0.5 };
		MeshAsset::MeshEntry entry;
		entry.base_index = 0;
		entry.base_vertex = 0;
		entry.material_index = 0;
		entry.num_indices = mp_base_cube->m_vao.vertex_data.indices.size();
		mp_base_cube->m_submeshes.push_back(entry);
		mp_base_cube->num_materials = 1;
		mp_base_cube->m_is_loaded = true;
	}


	void AssetManager::InitBaseTexture() {
		mp_base_tex = std::make_unique<Texture2D>("Base coded texture", 0);
		mp_base_tex->uuid = UUID(ORNG_BASE_TEX_ID);
		Texture2DSpec spec;
		spec.format = GL_RGB;
		spec.internal_format = GL_RGB8;
		spec.srgb_space = true;
		spec.width = 1;
		spec.height = 1;
		spec.wrap_params = GL_CLAMP_TO_EDGE;
		spec.min_filter = GL_NEAREST;
		spec.mag_filter = GL_NEAREST;
		mp_base_tex->SetSpec(spec);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, mp_base_tex->GetTextureHandle(), GL_TEXTURE0);
		unsigned char white_pixel[] = { 255, 255, 255, 255 };
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, white_pixel);
	}


	void SoundAsset::CreateSound() {
		if (auto result = AudioEngine::GetSystem()->createSound(source_filepath.c_str(), FMOD_DEFAULT | FMOD_3D | FMOD_LOOP_OFF, nullptr, &p_sound); result != FMOD_OK) {
			ORNG_CORE_ERROR("Error loading sound: '{0}', '{1}'", source_filepath, FMOD_ErrorString(result));
		}
	}



	SoundAsset::~SoundAsset() {
		if (p_sound)
			p_sound->release();
	}
}