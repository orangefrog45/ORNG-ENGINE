#include "pch/pch.h"
#include "core/AssetManager.h"
#include "core/CodedAssets.h"
#include "events/EventManager.h"
#include "core/Window.h"


namespace ORNG {

	void AssetManager::I_Init() {
		glfwWindowHint(GLFW_VISIBLE, 0);
		mp_loading_context = glfwCreateWindow(100, 100, "ASSET_LOADING_CONTEXT", nullptr, Window::GetGLFWwindow());

		// Each frame, check if any meshes have finished loading vertex data and load them into GPU if they have
		m_update_listener.OnEvent = [this](const Events::EngineCoreEvent& t_event) {
			if (t_event.event_type == Events::EngineCoreEvent::EventType::ENGINE_UPDATE) {

				for (int i = 0; i < m_mesh_loading_queue.size(); i++) {
					[[unlikely]] if (m_mesh_loading_queue[i].wait_for(std::chrono::nanoseconds(1)) == std::future_status::ready) {
						LoadMeshAssetIntoGL(m_mesh_loading_queue[i].get());
						m_mesh_loading_queue.erase(m_mesh_loading_queue.begin() + i);
						i--;
					}

				}
			}
		};

		Events::EventManager::RegisterListener(m_update_listener);

	}

	Texture2D* AssetManager::ICreateTexture2D(const Texture2DSpec& spec, uint64_t uuid) {

		for (auto* p_texture : m_2d_textures) {
			if (p_texture->GetSpec().filepath == spec.filepath) {
				ORNG_CORE_WARN("Texture '{0}' already created", spec.filepath);
				return p_texture;
			}
		}
		static std::vector<std::future<void>> futures;
		static std::mutex m;
		Texture2D* p_tex = uuid == 0 ? new Texture2D(spec.filepath.c_str()) : new Texture2D(spec.filepath.c_str(), uuid);
		m_2d_textures.push_back(p_tex);
		p_tex->SetSpec(spec);
		futures.push_back(std::async(std::launch::async, [p_tex, this, spec] {
			m.lock();
			glfwMakeContextCurrent(mp_loading_context);
			p_tex->LoadFromFile();
			glfwMakeContextCurrent(nullptr);
			m.unlock();
			}));

		return p_tex;
	}


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
		delete* it;
		m_2d_textures.erase(it);
	}

	Material* AssetManager::ICreateMaterial(uint64_t uuid) {
		Material* p_material = uuid == 0 ? new Material(&CodedAssets::GetBaseTexture()) : new Material(uuid);
		m_materials.push_back(p_material);
		return p_material;
	}

	Material* AssetManager::IGetMaterial(uint64_t id) {
		auto it = std::find_if(m_materials.begin(), m_materials.end(), [&](const auto* p_mat) {return p_mat->uuid() == id; });

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
		GL_StateManager::BindVAO(asset->m_vao);
		asset->PopulateBuffers();
		asset->importer.FreeScene();
		asset->m_is_loaded = true;
		asset->m_scene_materials = materials;
	}




	void AssetManager::IDeleteMaterial(uint64_t uuid) {
		auto it = std::ranges::find_if(m_materials, [uuid](auto* p_mat) {return p_mat->uuid() == uuid; });

		if (it == m_materials.end())
			return;

		Events::ProjectEvent e_event;
		e_event.data_payload = reinterpret_cast<uint8_t*>(*it);
		e_event.event_type = Events::ProjectEventType::MATERIAL_DELETED;
		Events::EventManager::DispatchEvent(e_event);

		delete* it;
		m_materials.erase(it);

	}

	void AssetManager::LoadMeshAssetIntoGL(MeshAsset* asset) {

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

		for (unsigned int i = 0; i < asset->p_scene->mNumMaterials; i++) {
			const aiMaterial* p_material = asset->p_scene->mMaterials[i];

			// Load material textures
			asset->m_original_materials[i].base_color_texture = LoadMeshAssetTexture(dir, aiTextureType_BASE_COLOR, p_material);
			asset->m_original_materials[i].normal_map_texture = LoadMeshAssetTexture(dir, aiTextureType_NORMALS, p_material);
			asset->m_original_materials[i].roughness_texture = LoadMeshAssetTexture(dir, aiTextureType_DIFFUSE_ROUGHNESS, p_material);
			asset->m_original_materials[i].metallic_texture = LoadMeshAssetTexture(dir, aiTextureType_METALNESS, p_material);
			asset->m_original_materials[i].ao_texture = LoadMeshAssetTexture(dir, aiTextureType_AMBIENT_OCCLUSION, p_material);

			// Load material properties 
			aiColor3D base_color(0.0f, 0.0f, 0.0f);
			if (p_material->Get(AI_MATKEY_BASE_COLOR, base_color) == aiReturn_SUCCESS) {
				asset->m_original_materials[i].base_color.r = base_color.r;
				asset->m_original_materials[i].base_color.g = base_color.g;
				asset->m_original_materials[i].base_color.b = base_color.b;
			}

			float roughness;
			float metallic;

			if (p_material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == aiReturn_SUCCESS)
				asset->m_original_materials[i].roughness = roughness;

			if (p_material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == aiReturn_SUCCESS)
				asset->m_original_materials[i].metallic = metallic;

			Material* p_scene_mat = CreateMaterial();
			uint64_t old_id = p_scene_mat->uuid();
			*p_scene_mat = asset->m_original_materials[i];
			p_scene_mat->uuid = UUID{ old_id };

			asset->m_scene_materials.push_back(p_scene_mat);

		}
		asset->PopulateBuffers();
		glFinish();
		asset->importer.FreeScene();
		asset->m_is_loaded = true;
	}

	void AssetManager::LoadMeshAsset(MeshAsset* p_asset) {
		Get().m_mesh_loading_queue.push_back(std::async(std::launch::async, [p_asset] {
			p_asset->LoadMeshData();
			return p_asset;
			}));
	};




	Texture2D* AssetManager::LoadMeshAssetTexture(const std::string& dir, aiTextureType type, const aiMaterial* p_material) {
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

		Events::ProjectEvent e_event;
		e_event.event_type = Events::ProjectEventType::MESH_DELETED;
		e_event.data_payload = reinterpret_cast<uint8_t*>(*it);
		Events::EventManager::DispatchEvent(e_event);
		delete* it;
		m_meshes.erase(it);
	};

}