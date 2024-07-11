#include "pch/pch.h"
#include "assets/AssetManager.h"
#include "events/EventManager.h"
#include "core/Window.h"
#include "rendering/Textures.h"
#include "rendering/MeshAsset.h"
#include "core/GLStateManager.h"
#include "scene/SceneSerializer.h"
#include "physics/Physics.h" // for material initialization
#include "yaml-cpp/yaml.h"
#include "rendering/EnvMapLoader.h"

// For glfwmakecontextcurrent
#include <GLFW/glfw3.h>




namespace ORNG {
	void AssetManager::I_Init() {
		glfwWindowHint(GLFW_VISIBLE, 0);
		mp_loading_context = glfwCreateWindow(100, 100, "ASSET_LOADING_CONTEXT", nullptr, Window::GetGLFWwindow());
		InitBaseAssets();

		// Each frame, check if any meshes have finished loading vertex data and load them into GPU if they have
		m_update_listener.OnEvent = [this](const Events::EngineCoreEvent& t_event) {
			if (t_event.event_type == Events::EngineCoreEvent::EventType::ENGINE_UPDATE && !m_mesh_loading_queue.empty()) {
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

						p_mesh->ClearCPU_VertexData();
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
			if (it->first < ORNG_NUM_BASE_ASSETS) {
				it++;
				continue;
			}

			HandleAssetDeletion(it->second);
			delete it->second;
			it = m_assets.erase(it);
		}
	}

	void AssetManager::IDeserializeAssetsFromBinaryPackage(const std::string& package_filepath) {
		std::ifstream s{ package_filepath, std::ios::binary };
		if (!s.is_open()) {
			ORNG_CORE_ERROR("Package file deserialization error: Cannot open {0} for reading", package_filepath);
			return;
		}

		bitsery::Deserializer<bitsery::InputStreamAdapter> des{ s };

		uint32_t num_textures, num_meshes, num_sounds;
		des.value4b(num_textures);
		des.value4b(num_meshes);
		des.value4b(num_sounds);

		std::vector<std::byte> bin_data;

		for (uint32_t i = 0; i < num_textures; i++) {
			Texture2D* p_tex = new Texture2D("");

			DeserializeTexture2D(*p_tex, bin_data, des);
			p_tex->LoadFromBinary(bin_data);
			AddAsset(p_tex);
		}

		for (uint32_t i = 0; i < num_meshes; i++) {
			MeshAsset* p_mesh = new MeshAsset("");

			DeserializeMeshAsset(*p_mesh, des);
			AssetManager::LoadMeshAsset(p_mesh);
			AddAsset(p_mesh);
		}

		for (uint32_t i = 0; i < num_sounds; i++) {
			SoundAsset* p_sound = new SoundAsset("");

			DeserializeSoundAsset(*p_sound, bin_data, des);
			p_sound->CreateSoundFromBinary(bin_data);
			AddAsset(p_sound);
		}
	}

	void AssetManager::ICreateBinaryAssetPackage(const std::string& output_path) {
		std::ofstream s{ output_path, s.binary | s.trunc | s.out };

		if (!s.is_open()) {
			ORNG_CORE_ERROR("Binary serialization error: Cannot open {0} for writing", output_path);
			return;
		}

		bitsery::Serializer<bitsery::OutputStreamAdapter> ser{ s };

		auto texture_view = GetView<Texture2D>();
		auto mesh_view = GetView<MeshAsset>();
		auto sound_view = GetView<SoundAsset>();
		
		// Begin layout with number of assets
		ser.value4b(static_cast<uint32_t>(texture_view.size()));
		ser.value4b(static_cast<uint32_t>(mesh_view.size()));
		ser.value4b(static_cast<uint32_t>(sound_view.size()));
		
		// Serialize asset specs
		for (auto* p_texture : texture_view) {
			SerializeTexture2D(*p_texture, p_texture->filepath, ser);
		}
		for (auto* p_mesh : mesh_view) {
			ser.object(*p_mesh);
		}
		for (auto* p_sound : sound_view) {
			SerializeSoundAsset(*p_sound, p_sound->filepath, ser);
		}

		// flush to writer
		ser.adapter().flush();
		s.close();
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

		asset->OnLoadIntoGL();

		asset->m_is_loaded = true;
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

	void AssetManager::LoadExternalBaseAssets(const std::string& project_dir) {
		m_assets.erase(ORNG_BASE_SOUND_ID);
		mp_base_sound = std::make_unique<SoundAsset>(project_dir + "res\\core-res\\audio\\mouse-click.mp3");
		mp_base_sound->uuid = UUID<uint64_t>(ORNG_BASE_SOUND_ID);
		mp_base_sound->source_filepath = project_dir + "res\\core-res\\audio\\mouse-click.mp3";
		mp_base_sound->CreateSoundFromFile();
		AddAsset(&*mp_base_sound);

		m_assets.erase(ORNG_BASE_SPHERE_ID);
		mp_base_sphere.release();
		mp_base_sphere = std::make_unique<MeshAsset>("res/meshes/Sphere.obj");
		DeserializeAssetBinary("res/core-res/meshes/Sphere.obj.bin", *mp_base_sphere);
		mp_base_sphere->PopulateBuffers();
		mp_base_sphere->m_is_loaded = true;
		mp_base_sphere->uuid = UUID<uint64_t>(ORNG_BASE_SPHERE_ID);
		AddAsset(&*mp_base_sphere);
	}


	void AssetManager::SerializeTexture2D(Texture2D& tex, const std::string& output_filepath, bitsery::Serializer<bitsery::OutputStreamAdapter>& ser) {

		std::vector<std::byte> texture_data;

		if (FileExists(tex.m_spec.filepath)) { // Texture data still on disk, read from this
			ReadBinaryFile(tex.m_spec.filepath, texture_data);
		}
		else if (FileExists(output_filepath)) { // Texture has been previously serialized into a binary file and data resides there
			Texture2D dummy{ "", 0 };
			DeserializeAssetBinary(output_filepath, tex, &texture_data);
			tex.m_spec.filepath = ""; // No longer need to store reference to first asset path as data will be available from this branch
		}

		ser.object(tex.m_spec);
		ser.object(tex.uuid);
		ser.text1b(tex.filepath, ORNG_MAX_FILEPATH_SIZE);
		ser.container1b(texture_data, UINT64_MAX);
	 }


	void AssetManager::DeserializeTexture2D(Texture2D& tex, std::vector<std::byte>& data, bitsery::Deserializer<bitsery::InputStreamAdapter>& des) {
		des.object(tex.m_spec);
		des.object(tex.uuid);
		des.text1b(tex.filepath, ORNG_MAX_FILEPATH_SIZE);
		des.container1b(data, UINT64_MAX);
	}

	void AssetManager::DeserializeSoundAsset(SoundAsset& sound, std::vector<std::byte>& raw_data, bitsery::Deserializer<bitsery::InputStreamAdapter>& des) {
		des.object(sound.uuid);
		des.text1b(sound.filepath, ORNG_MAX_FILEPATH_SIZE);
		des.container1b(raw_data, UINT64_MAX);
	}

	void AssetManager::SerializeSoundAsset(SoundAsset& sound, const std::string& output_filepath, bitsery::Serializer<bitsery::OutputStreamAdapter>& ser) {
		std::vector<std::byte> sound_data;

		if (FileExists(sound.source_filepath)) { // Data still on disk, read from this
			ReadBinaryFile(sound.source_filepath, sound_data);
		}
		else if (FileExists(output_filepath)) { // Sound has been previously serialized into a binary file and data resides there
			SoundAsset dummy{ "" };
			DeserializeAssetBinary(output_filepath, sound, &sound_data);
		}

		ser.object(sound.uuid);
		ser.text1b(sound.filepath, ORNG_MAX_FILEPATH_SIZE);
		ser.container1b(sound_data, UINT64_MAX);
	}

	void AssetManager::DeserializeMeshAsset(MeshAsset& mesh, bitsery::Deserializer<bitsery::InputStreamAdapter>& des) {
		des.object(mesh.m_vao);
		des.object(mesh.m_aabb);
		uint32_t size;
		des.value4b(size);
		mesh.m_submeshes.resize(size);
		for (int i = 0; i < size; i++) {
			des.object(mesh.m_submeshes[i]);
		}
		des.value1b(mesh.num_materials);
		des.object(mesh.uuid);
		des.text1b(mesh.filepath, ORNG_MAX_FILEPATH_SIZE);
	}

	void AssetManager::DeserializeMaterialAsset(Material& data, bitsery::Deserializer<bitsery::InputStreamAdapter>& des) {
		des.object(data.base_color);
		des.value1b(data.render_group);
		des.value4b(data.roughness);
		des.value4b(data.metallic);
		des.value4b(data.ao);
		des.value4b(data.emissive_strength);
		uint64_t texid;
		des.value8b(texid);
		if (texid != 0) data.base_color_texture = GetAsset<Texture2D>(texid);
		des.value8b(texid);
		if (texid != 0) data.normal_map_texture = GetAsset<Texture2D>(texid);
		des.value8b(texid);
		if (texid != 0) data.metallic_texture = GetAsset<Texture2D>(texid);
		des.value8b(texid);
		if (texid != 0) data.roughness_texture = GetAsset<Texture2D>(texid);
		des.value8b(texid);
		if (texid != 0) data.ao_texture = GetAsset<Texture2D>(texid);
		des.value8b(texid);
		if (texid != 0) data.displacement_texture = GetAsset<Texture2D>(texid);
		des.value8b(texid);
		if (texid != 0) data.emissive_texture = GetAsset<Texture2D>(texid);

		des.value4b(data.parallax_layers);
		des.object(data.tile_scale);
		des.text1b(data.name, ORNG_MAX_NAME_SIZE);
		des.object(data.uuid);
		des.object(data.spritesheet_data);

		uint32_t flags;
		des.value4b(flags);
		data.flags = (MaterialFlags)flags;
		des.value4b(data.displacement_scale);
	}

	void AssetManager::DeserializePhysxMaterialAsset(PhysXMaterialAsset& data, bitsery::Deserializer<bitsery::InputStreamAdapter>& des) {
		des.object(data.uuid);
		des.container1b(data.name, ORNG_MAX_FILEPATH_SIZE);

		float sf;
		float df;
		float r;

		des.value4b(sf);
		des.value4b(df);
		des.value4b(r);

		InitPhysXMaterialAsset(data);
		data.p_material->setDynamicFriction(df);
		data.p_material->setStaticFriction(sf);
		data.p_material->setRestitution(r);
	}


	void AssetManager::LoadAssetsFromProjectPath(const std::string& project_dir, bool precompiled_scripts) {
		std::string texture_folder = project_dir + "\\res\\textures\\";
		std::string mesh_folder = project_dir + "\\res\\meshes\\";
		std::string audio_folder = project_dir + "\\res\\audio\\";
		std::string material_folder = project_dir + "\\res\\materials\\";
		std::string prefab_folder = project_dir + "\\res\\prefabs\\";
		std::string script_folder = project_dir + "\\res\\scripts\\";
		std::string physx_mat_folder = project_dir + "\\res\\physx-materials\\";


		Texture2DSpec default_spec;
		default_spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
		default_spec.mag_filter = GL_LINEAR;
		default_spec.generate_mipmaps = true;
		default_spec.storage_type = GL_UNSIGNED_BYTE;

		for (const auto& entry : std::filesystem::recursive_directory_iterator(texture_folder)) {
			std::string path = entry.path().string();

			if (entry.is_directory() || path.find("diffuse_prefilter") != std::string::npos || entry.path().extension() != ".otex") // Skip serialized diffuse prefilter
				continue;

			auto rel_path = path.substr(path.rfind("\\res\\") + 1);
			default_spec.filepath = rel_path;
			auto* p_tex = new Texture2D(path);
			std::vector<std::byte> binary_data;
			auto& spec = p_tex->GetSpec();

			DeserializeAssetBinary(path, *p_tex, &binary_data);
			p_tex->filepath = rel_path;
			AddAsset(p_tex);
			p_tex->LoadFromBinary(binary_data);
			DispatchAssetEvent(Events::AssetEventType::TEXTURE_LOADED, reinterpret_cast<uint8_t*>(p_tex));
		}

		for (const auto& entry : std::filesystem::recursive_directory_iterator(mesh_folder)) {
			if (entry.is_directory() || entry.path().extension() != ".omesh")
				continue;

			std::string str_path = entry.path().string();
			std::string rel_path = ".\\" + str_path.substr(str_path.rfind("res\\meshes"));
			auto* p_mesh = new MeshAsset(rel_path);
			DeserializeAssetBinary(rel_path, *p_mesh);
			p_mesh->filepath = rel_path;
			AddAsset(p_mesh);
			LoadMeshAssetIntoGL(p_mesh);
		}


		for (const auto& entry : std::filesystem::recursive_directory_iterator(audio_folder)) {
			auto path = entry.path().string();
			if (entry.is_directory() || !(entry.path().extension() == ".osound"))
				continue;
			else {
				std::string rel_path = ".\\" + path.substr(path.rfind("res\\audio"));
				auto* p_sound = new SoundAsset(path);
				std::vector<std::byte> raw_sound_data;
				DeserializeAssetBinary(path, *p_sound, &raw_sound_data);
				p_sound->filepath = rel_path;
				p_sound->CreateSoundFromBinary(raw_sound_data);
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
				

				p_prefab->node = YAML::Load(p_prefab->serialized_content);

#ifndef ORNG_EDITOR_LAYER 
				p_prefab->serialized_content.clear();
#endif
			}
		}

		for (const auto& entry : std::filesystem::recursive_directory_iterator(script_folder)) {
			auto path = entry.path();
			std::string path_string = path.string();
			if (entry.is_directory() || (!precompiled_scripts && path.extension() != ".cpp") || (precompiled_scripts && path.extension() != ".dll") || path.string().find("scripts\\includes") != std::string::npos)
				continue;
			else {
				if (precompiled_scripts) {
#ifdef NDEBUG
					if (path_string.find("debug") != std::string::npos)
						continue;
#else
					if (path_string.find("release") != std::string::npos)
						continue;
#endif
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


		for (const auto& entry : std::filesystem::recursive_directory_iterator(physx_mat_folder)) {
			auto path = entry.path();
			if (entry.is_directory() || path.extension() != ".opmat")
				continue;
			else {
				std::string rel_path = ".\\" + path.string().substr(path.string().rfind("res\\physx-materials"));
				auto* p_mat = new PhysXMaterialAsset(rel_path);
				DeserializeAssetBinary(rel_path, *p_mat);
				AddAsset(p_mat);
			}
		}
	}


	void AssetManager::ISerializeAssets() {
		// Serialize all assets currently loaded into asset manager
		// Meshes and prefabs are overlooked here as they are serialized automatically upon being loaded into the engine
		for (auto* p_texture : GetView<Texture2D>()) {
			SerializeAssetToBinaryFile(*p_texture, ".\\res\\textures\\" + p_texture->filepath.substr(p_texture->filepath.rfind("\\") + 1));
		}

		for (auto* p_mat : GetView<Material>()) {
			SceneSerializer::SerializeBinary(".\\res\\materials\\" + std::format("{}", p_mat->uuid()) + ".omat", *p_mat);
		}

		for (auto* p_sound : GetView<SoundAsset>()) {
			std::string fn = p_sound->filepath.substr(p_sound->filepath.rfind("\\") + 1);
			fn = ".\\res\\audio\\" + ReplaceFileExtension(fn, ".osound");
			SerializeAssetToBinaryFile(*p_sound, fn);
		}

		for (auto* p_mat : GetView<PhysXMaterialAsset>()) {
			SceneSerializer::SerializeBinary(".\\res\\physx-materials\\" + std::format("{}", p_mat->uuid()) + ".opmat", *p_mat);
		}
	}

	void AssetManager::InitPhysXMaterialAsset(PhysXMaterialAsset& asset) {
		asset.p_material = Physics::GetPhysics()->createMaterial(0.5, 0.5, 0.5);
	}


	void AssetManager::InitBaseAssets() {
		InitBaseCube();
		InitBaseTexture();
		InitBase3DQuad();
		
		LoadExternalBaseAssets("");

		mp_base_material = std::make_unique<Material>((uint64_t)ORNG_BASE_MATERIAL_ID);
		mp_base_material->name = "Base material";

		auto symbols = ScriptSymbols("");
		mp_base_script = std::make_unique<ScriptAsset>(symbols);
		mp_base_script->uuid = UUID<uint64_t>(ORNG_BASE_SCRIPT_ID);
		mp_base_cube->uuid = UUID<uint64_t>(ORNG_BASE_MESH_ID);
		mp_base_tex->uuid = UUID<uint64_t>(ORNG_BASE_TEX_ID);
		mp_base_material->uuid = UUID<uint64_t>(ORNG_BASE_MATERIAL_ID);
		mp_base_brdf_lut = std::make_unique<Texture2D>("Base BRDF LUT");
		mp_base_brdf_lut->uuid = UUID<uint64_t>(ORNG_BASE_BRDF_LUT_ID);

		EnvMapLoader::LoadBRDFConvolution(*mp_base_brdf_lut);

		AddAsset(&*mp_base_cube);
		AddAsset(&*mp_base_tex);
		AddAsset(&*mp_base_material);
		AddAsset(&*mp_base_script);
		AddAsset(&*mp_base_quad);

		mp_base_physx_material = std::make_unique<PhysXMaterialAsset>("BASE");
		mp_base_physx_material->uuid = UUID<uint64_t>(ORNG_BASE_PHYSX_MATERIAL_ID);
		mp_base_physx_material->p_material = Physics::GetPhysics()->createMaterial(0.75f, 0.75f, 0.6f);
		AddAsset(&*mp_base_physx_material);
	}


	void AssetManager::IOnShutdown() {
		ClearAll();
		Get().mp_base_material.release();
		Get().mp_base_sound.release();
		Get().mp_base_tex.release();
		Get().mp_base_cube.release();
		Get().mp_base_sphere.release();
		Get().mp_base_physx_material->p_material->release();
		Get().mp_base_physx_material.release();
		Get().mp_base_quad.release();
		Get().mp_base_script.release();
	};



	void AssetManager::InitBaseCube() {
		mp_base_cube = std::make_unique<MeshAsset>("Base cube");
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

			1.0, 1.0, 1.0, // Vertex 13
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

		mp_base_cube->num_indices = mp_base_cube->m_vao.vertex_data.indices.size();
		mp_base_cube->uuid = UUID<uint64_t>(ORNG_BASE_MESH_ID);
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

	void AssetManager::InitBase3DQuad() {
		mp_base_quad = std::make_unique<MeshAsset>("quad");

		mp_base_quad->m_vao.vertex_data.positions = {
			-0.5, -0.5, 0,
			-0.5, 0.5, 0,
			0.5, -0.5, 0,
			0.5, 0.5, 0,
		};

		mp_base_quad->m_vao.vertex_data.normals = {
			0, 0, 1,
			0, 0, 1,
			0, 0, 1,
			0, 0, 1,
		};

		mp_base_quad->m_vao.vertex_data.tangents = {
			1, 0, 0,
			1, 0, 0,
			1, 0, 0,
			1, 0, 0,
		};

		mp_base_quad->m_vao.vertex_data.tex_coords = {
		  0.0, 0.0,
			0.0, 1.0,
			1.0, 0.0,
			1.0, 1.0
		};
		mp_base_quad->m_vao.vertex_data.indices = {
			0, 2, 1, 3, 1, 2
		};
		mp_base_quad->num_materials = 1;

		MeshAsset::MeshEntry entry;
		entry.base_index = 0;
		entry.base_vertex = 0;
		entry.material_index = 0;
		entry.num_indices = mp_base_quad->m_vao.vertex_data.indices.size();

		mp_base_quad->m_submeshes.push_back(entry);
		mp_base_quad->m_is_loaded = true;
		mp_base_quad->uuid = UUID<uint64_t>(ORNG_BASE_QUAD_ID);

		mp_base_quad->m_vao.FillBuffers();
	}

	void AssetManager::InitBaseTexture() {
		mp_base_tex = std::make_unique<Texture2D>("Base coded texture", 0);
		mp_base_tex->uuid = UUID<uint64_t>(ORNG_BASE_TEX_ID);
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
}