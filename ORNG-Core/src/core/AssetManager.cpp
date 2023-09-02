#include "pch/pch.h"
#include "core/AssetManager.h"
#include "core/CodedAssets.h"
#include "events/EventManager.h"
#include "core/Window.h"
#include "assimp/scene.h"
#include "rendering/Textures.h"
#include "rendering/MeshAsset.h"
#include "audio/AudioEngine.h"
#include <fmod.hpp>
#include <fmod_errors.h>

// For glfwmakecontextcurrent
#include <GLFW/glfw3.h>


namespace ORNG {

	void AssetManager::I_Init() {
		glfwWindowHint(GLFW_VISIBLE, 0);
		mp_loading_context = glfwCreateWindow(100, 100, "ASSET_LOADING_CONTEXT", nullptr, Window::GetGLFWwindow());

		// Each frame, check if any meshes have finished loading vertex data and load them into GPU if they have
		m_update_listener.OnEvent = [this](const Events::EngineCoreEvent& t_event) {
			if (t_event.event_type == Events::EngineCoreEvent::EventType::ENGINE_UPDATE) {

				for (int i = 0; i < m_mesh_loading_queue.size(); i++) {
					[[unlikely]] if (m_mesh_loading_queue[i].wait_for(std::chrono::nanoseconds(1)) == std::future_status::ready) {
						auto mesh_package = m_mesh_loading_queue[i].get();

						m_mesh_loading_queue.erase(m_mesh_loading_queue.begin() + i);
						i--;

						LoadMeshAssetIntoGL(mesh_package.p_asset, mesh_package.materials);

						Events::AssetEvent e_event;
						e_event.event_type = Events::AssetEventType::MESH_LOADED;
						e_event.data_payload = reinterpret_cast<uint8_t*>(mesh_package.p_asset);
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
					auto mesh_package = m_mesh_loading_queue[i].get();

					m_mesh_loading_queue.erase(m_mesh_loading_queue.begin() + i);
					i--;

					LoadMeshAssetIntoGL(mesh_package.p_asset, mesh_package.materials);
					for (auto& future : m_texture_futures) {
						future.get();
					}
					m_texture_futures.clear();
					DispatchAssetEvent(Events::AssetEventType::MESH_LOADED, reinterpret_cast<uint8_t*>(mesh_package.p_asset));
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
		while (!m_materials.empty()) {
			DeleteMaterial(m_materials[0]);
		}
		while (!m_2d_textures.empty()) {
			DeleteTexture(m_2d_textures[0]);
		}
		while (!m_meshes.empty()) {
			DeleteMeshAsset(m_meshes[0]);
		}
		while (!m_scripts.empty()) {
			DeleteScriptAsset(m_scripts.begin()->first);
		}

	}

	Texture2D* AssetManager::ICreateTexture2D(const Texture2DSpec& spec, uint64_t uuid) {

		for (auto* p_texture : m_2d_textures) {
			if (p_texture->GetSpec().filepath == spec.filepath) {
				ORNG_CORE_WARN("Texture '{0}' already created", spec.filepath);
				return p_texture;
			}
		}
		static std::mutex m;
		Texture2D* p_tex = uuid == 0 ? new Texture2D(spec.filepath.c_str()) : new Texture2D(spec.filepath.c_str(), uuid);
		m_2d_textures.push_back(p_tex);
		p_tex->SetSpec(spec);
		m_texture_futures.push_back(std::async(std::launch::async, [p_tex, this, spec] {
			m.lock();
			glfwMakeContextCurrent(mp_loading_context);
			p_tex->LoadFromFile();
			glfwMakeContextCurrent(nullptr);
			m.unlock();
			}));

		DispatchAssetEvent(Events::AssetEventType::TEXTURE_LOADED, reinterpret_cast<uint8_t*>(p_tex));


		return p_tex;
	}

	void AssetManager::DeleteMeshAsset(MeshAsset* p_asset) {
		Get().IDeleteMeshAsset(p_asset->uuid());
	}

	void AssetManager::DeleteTexture(Texture2D* p_tex) { Get().IDeleteTexture(p_tex->uuid()); }

	Texture2D* AssetManager::IGetTexture(uint64_t uuid) {
		auto it = std::find_if(m_2d_textures.begin(), m_2d_textures.end(), [&](const auto* p_tex) {return p_tex->uuid() == uuid; });

		if (it == m_2d_textures.end()) {
			ORNG_CORE_WARN("Texture with ID '{0}' does not exist, not found", uuid);
			return nullptr;
		}

		return *it;
	}

	void AssetManager::IDeleteTexture(uint64_t uuid) {
		auto it = std::find_if(m_2d_textures.begin(), m_2d_textures.end(), [&](const auto* p_tex) {return p_tex->uuid() == uuid; });

		if (it == m_2d_textures.end()) {
			ORNG_CORE_WARN("Texture with ID '{0}' does not exist, not deleted", uuid);
			return;
		}

		// If any materials use this texture, remove it from them
		for (auto* p_material : m_materials) {
			p_material->base_color_texture = p_material->base_color_texture == *it ? nullptr : p_material->base_color_texture;
			p_material->normal_map_texture = p_material->normal_map_texture == *it ? nullptr : p_material->normal_map_texture;
			p_material->emissive_texture = p_material->emissive_texture == *it ? nullptr : p_material->emissive_texture;
			p_material->displacement_texture = p_material->displacement_texture == *it ? nullptr : p_material->displacement_texture;
			p_material->metallic_texture = p_material->metallic_texture == *it ? nullptr : p_material->metallic_texture;
			p_material->roughness_texture = p_material->roughness_texture == *it ? nullptr : p_material->roughness_texture;
		}

		DispatchAssetEvent(Events::AssetEventType::TEXTURE_DELETED, reinterpret_cast<uint8_t*>(*it));

		delete* it;
		m_2d_textures.erase(it);
	}



	Material* AssetManager::ICreateMaterial(uint64_t uuid) {
		Material* p_material = uuid == 0 ? new Material(&CodedAssets::GetBaseTexture()) : new Material(uuid);
		m_materials.push_back(p_material);

		DispatchAssetEvent(Events::AssetEventType::MATERIAL_LOADED, reinterpret_cast<uint8_t*>(p_material));

		return p_material;
	}

	Material* AssetManager::IGetMaterial(uint64_t id) {
		auto it = std::find_if(m_materials.begin(), m_materials.end(), [id](const auto* p_mat) {return p_mat->uuid() == id; });

		if (it == m_materials.end()) {
			ORNG_CORE_ERROR("Material with ID '{0}' does not exist, not found", id);
			return &m_replacement_material;
		}

		return *it;

	}



	void AssetManager::LoadMeshAssetPreExistingMaterials(MeshAsset* asset, std::vector<Material*>& materials) {
		if (asset->m_is_loaded) {
			ORNG_CORE_ERROR("Mesh '{0}' is already loaded", asset->m_filename);
			return;
		}

		Get().m_mesh_loading_queue.emplace_back(std::async(std::launch::async, [asset, materials] {
			asset->LoadMeshData();
			return MeshAssetPackage{ asset, materials };
			}));

	}




	void AssetManager::IDeleteMaterial(uint64_t uuid) {
		auto it = std::ranges::find_if(m_materials, [uuid](auto* p_mat) {return p_mat->uuid() == uuid; });

		if (it == m_materials.end())
			return;


		DispatchAssetEvent(Events::AssetEventType::MATERIAL_DELETED, reinterpret_cast<uint8_t*>(*it));

		delete* it;
		m_materials.erase(it);

	}

	void AssetManager::LoadMeshAssetIntoGL(MeshAsset* asset, std::vector<Material*>& materials) {

		if (asset->m_is_loaded) {
			ORNG_CORE_ERROR("Mesh '{0}' is already loaded", asset->m_filename);
			return;
		}

		GL_StateManager::BindVAO(asset->GetVAO());

		// Get directory used for finding material textures
		std::string::size_type slash_index = asset->GetFilename().find_last_of("/");
		std::string dir;

		if (slash_index == std::string::npos) {
			dir = ".";
		}
		else if (slash_index == 0) {
			dir = "/";
		}
		else {
			dir = asset->GetFilename().substr(0, slash_index);
		}

		if (materials.empty()) {
			for (unsigned int i = 0; i < asset->p_scene->mNumMaterials; i++) {
				const aiMaterial* p_material = asset->p_scene->mMaterials[i];
				Material* p_new_material = Get().ICreateMaterial();

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
					DeleteMaterial(p_new_material);
					asset->m_material_assets.emplace_back(&Get().m_replacement_material);
				}
				else {
					asset->m_material_assets.emplace_back(p_new_material);
				}
			}
		}
		else {
			asset->m_material_assets = materials;

		}

		asset->PopulateBuffers();
		asset->m_is_loaded = true;
	}

	void AssetManager::LoadMeshAsset(MeshAsset* p_asset) {
		Get().m_mesh_loading_queue.emplace_back(std::async(std::launch::async, [p_asset] {
			p_asset->LoadMeshData();
			return MeshAssetPackage{ p_asset, {} };
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

				full_path = dir + "/" + p;
				std::ranges::for_each(full_path, [](char& c) {if (c == '\\') c = '/'; });

				Texture2DSpec base_spec;
				base_spec.generate_mipmaps = true;
				base_spec.mag_filter = GL_LINEAR;
				base_spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
				base_spec.filepath = full_path;
				base_spec.srgb_space = type == aiTextureType_BASE_COLOR ? true : false;

				p_tex = CreateTexture2D(base_spec);

			}

		}

		return p_tex;
	}



	MeshAsset* AssetManager::ICreateMeshAsset(const std::string& filename, uint64_t uuid) {
		for (auto mesh_data : m_meshes) {
			if (mesh_data->GetFilename() == filename) {
				ORNG_CORE_WARN("Mesh asset already loaded in: {0}", filename);
				return mesh_data;
			}
		}


		MeshAsset* p_asset = uuid == 0 ? new MeshAsset(filename) : new MeshAsset(filename, uuid);
		m_meshes.push_back(p_asset);
		return p_asset;
	}




	MeshAsset* AssetManager::IGetMeshAsset(uint64_t uuid) {
		auto it = std::ranges::find_if(m_meshes, [&](const auto* p_tex) {return p_tex->uuid() == uuid; });

		if (it == m_meshes.end()) {
			ORNG_CORE_TRACE("Mesh with ID '{0}' does not exist, using CodedCube as replacement", uuid);
			return &CodedAssets::GetCubeAsset();
		}

		return *it;
	}



	void AssetManager::IDeleteMeshAsset(uint64_t uuid) {
		auto it = std::ranges::find_if(m_meshes, [&](const auto* p_tex) {return p_tex->uuid() == uuid; });

		if (it == m_meshes.end()) {
			ORNG_CORE_TRACE("Mesh with ID '{0}' does not exist, using CodedCube as replacement", uuid);
			return;
		}


		DispatchAssetEvent(Events::AssetEventType::MESH_DELETED, reinterpret_cast<uint8_t*>(*it));

		delete* it;
		m_meshes.erase(it);
	};


	ScriptSymbols* AssetManager::AddScriptAsset(const std::string& filepath) {
		auto symbols = ScriptingEngine::GetSymbolsFromScriptCpp(filepath);

		if (!symbols.loaded) {
			ORNG_CORE_ERROR("Error adding script asset, symbols failed to be loaded by script engine");
			return nullptr;
		}

		ScriptSymbols* p_symbols = new ScriptSymbols();
		*p_symbols = symbols;

		Get().m_scripts[filepath] = p_symbols;
		return p_symbols;
	}


	ScriptSymbols* AssetManager::GetScriptAsset(const std::string& filepath) {
		if (!Get().m_scripts.contains(filepath)) {
			ORNG_CORE_TRACE("Failed to get script asset '{0}', not found in map", filepath);
			return nullptr;
		}
		else {
			return Get().m_scripts[filepath];
		}
	}

	bool AssetManager::DeleteScriptAsset(const std::string& filepath) {
		if (!GetScriptAsset(filepath)) {
			ORNG_CORE_TRACE("Failed to delete script asset '{0}', not found in map", filepath);
			return false;
		}
		else {
			delete Get().m_scripts[filepath];
			Get().m_scripts.erase(filepath);
			ScriptingEngine::UnloadScriptDLL(filepath);
			return true;
		}
	}


	SoundAsset* AssetManager::AddSoundAsset(const std::string& filepath) {
		if (auto* p_existing_sound = GetSoundAsset(filepath)) {
			return p_existing_sound;
		}
		else {
			FMOD::Sound* p_fmod_sound = nullptr;
			AudioEngine::GetSystem()->createSound(filepath.c_str(), FMOD_DEFAULT, nullptr, &p_fmod_sound);
			auto* p_sound = new SoundAsset(p_fmod_sound, filepath);
			Get().m_sound_assets[filepath] = p_sound;
			return p_sound;
		}
	}


	SoundAsset* AssetManager::GetSoundAsset(const std::string& filepath) {
		if (!Get().m_sound_assets.contains(filepath)) {
			return nullptr;
		}
		else {
			return Get().m_sound_assets[filepath];
		}
	}


	void AssetManager::DeleteSoundAsset(SoundAsset* p_asset) {
		if (auto* p_existing_sound = GetSoundAsset(p_asset->filepath)) {
			delete p_existing_sound;
			Get().m_sound_assets.erase(p_asset->filepath);
		}
		else {
			ORNG_CORE_ERROR("Asset manager failed to delete sound asset '{0}', not found", p_asset->filepath);
		}
	}

	SoundAsset::~SoundAsset() {
		p_sound->release();
	}
}