#include "pch/pch.h"

#include "util/TimeStep.h"
#include "scene/Scene.h"
#include "util/util.h"
#include "scene/MeshInstanceGroup.h"
#include "rendering/Renderer.h"
#include "core/GLStateManager.h"
#include "scene/SceneEntity.h"
#include "components/CameraComponent.h"
#include "../extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD.h"
#include "physics/Physics.h"
#include "scene/SceneSerializer.h"


namespace ORNG {


	Scene::~Scene() {
		UnloadScene();
	}

	Scene::Scene() {


	}


	void Scene::Update(float ts) {

		m_physics_system.OnUpdate(ts);
		for (auto* script : m_script_components) {
			script->OnUpdate();
		}

		m_mesh_component_manager.OnUpdate();
		m_pointlight_component_manager.OnUpdate();
		m_spotlight_component_manager.OnUpdate();
		m_transform_component_manager.OnUpdate();
		m_camera_system.OnUpdate();

		m_terrain.UpdateTerrainQuadtree(m_camera_system.p_active_camera->mp_transform->GetPosition());
	}










	ScriptComponent* Scene::AddScriptComponent(SceneEntity* p_entity) {
		if (GetComponent<ScriptComponent>(p_entity->GetID())) {
			OAR_CORE_WARN("Script component not added, entity '{0}' already has a script component", p_entity->name);
			return nullptr;
		}

		auto* p_comp = new ScriptComponent(p_entity);
		m_script_components.push_back(p_comp);
		return p_comp;
	}




	MeshComponent* Scene::AddMeshComponent(SceneEntity* p_entity, const std::string& filename) {

		// Currently any mesh components inside scene are auto-instanced and grouped together in MeshInstanceGroups, depending on shader, materials and asset
		//check if mesh data is already available for the instance group about to be created if new component does not have appropiate instance group
		int mesh_asset_index = -1;
		for (int i = 0; i < m_mesh_assets.size(); i++) {
			if (m_mesh_assets[i]->GetFilename() == filename) {
				mesh_asset_index = i;
				break;
			}
		}

		MeshAsset* p_mesh_data;
		if (mesh_asset_index == -1) {
			p_mesh_data = CreateMeshAsset(filename);
			p_mesh_data->LoadMeshData();
			LoadMeshAssetIntoGPU(p_mesh_data);
		}
		else {
			p_mesh_data = m_mesh_assets[mesh_asset_index];
		}

		return m_mesh_component_manager.AddComponent(p_entity, p_mesh_data);
	}




	Texture2D* Scene::LoadMeshAssetTexture(const std::string& dir, aiTextureType type, const aiMaterial* p_material) {
		Texture2D* p_tex = nullptr;


		if (p_material->GetTextureCount(type) > 0) {
			aiString path;

			if (p_material->GetTexture(type, 0, &path, NULL, NULL, NULL, NULL, NULL) == aiReturn_SUCCESS) {
				std::string p(path.data);
				std::string full_path;

				if (p.starts_with(".\\"))
					p = p.substr(2, p.size() - 2);

				full_path = dir + "/" + p;

				Texture2DSpec base_spec;
				base_spec.generate_mipmaps = true;
				base_spec.mag_filter = GL_LINEAR;
				base_spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
				base_spec.filepath = full_path;
				base_spec.srgb_space = type == aiTextureType_BASE_COLOR ? true : false;

				p_tex = CreateTexture2DAsset(base_spec);

			}

		}

		return p_tex;
	}



	SceneEntity* Scene::GetEntity(uint64_t uuid) {
		auto it = std::find_if(m_entities.begin(), m_entities.end(), [&](const auto* p_entity) {return p_entity->GetID() == uuid; });
		return it == m_entities.end() ? nullptr : *it;
	}




	void Scene::LoadScene(const std::string& filepath) {
		TimeStep time = TimeStep(TimeStep::TimeUnits::MILLISECONDS);

		FastNoiseSIMD* noise = FastNoiseSIMD::NewFastNoiseSIMD();
		noise->SetFrequency(0.05f);
		noise->SetCellularReturnType(FastNoiseSIMD::Distance);
		noise->SetNoiseType(FastNoiseSIMD::PerlinFractal);

		Texture2DSpec base_spec;
		base_spec.generate_mipmaps = true;
		base_spec.mag_filter = GL_LINEAR;
		base_spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
		base_spec.filepath = "./res/textures/missing_texture.png";
		auto* p_tex = CreateTexture2DAsset(base_spec, DEFAULT_BASE_COLOR_TEX_ID);

		Material* default_material = CreateMaterial(BASE_MATERIAL_ID);
		default_material->name = "Default";
		default_material->base_color = glm::vec3(1);
		default_material->base_color_texture = p_tex;

		m_skybox.LoadEnvironmentMap("./res/textures/belfast_sunset_puresky_4k.hdr");
		m_global_fog.SetNoise(noise);
		m_terrain.Init(BASE_MATERIAL_ID);
		m_physics_system.OnLoad();
		m_mesh_component_manager.OnLoad();
		m_spotlight_component_manager.OnLoad();
		m_pointlight_component_manager.OnLoad();

		SceneSerializer::DeserializeScene(*this, filepath);
		OAR_CORE_INFO("Scene loaded in {0}ms", time.GetTimeInterval());
	}




	void Scene::UnloadScene() {
		OAR_CORE_INFO("Unloading scene...");
		m_physics_system.OnUnload();
		m_mesh_component_manager.OnUnload();
		m_spotlight_component_manager.OnUnload();
		m_pointlight_component_manager.OnUnload();
		m_camera_system.OnUnload();
		m_transform_component_manager.OnUnload();

		for (auto* script : m_script_components) {
			delete script;
		}
		for (MeshAsset* mesh_data : m_mesh_assets) {
			delete mesh_data;
		}
		for (auto* entity : m_entities) {
			delete entity;
		}
		for (auto* material : m_materials) {
			delete material;
		}
		for (auto* texture : m_texture_2d_assets) {
			if (texture)
				delete texture;
		}

		m_script_components.clear();
		m_mesh_assets.clear();
		m_entities.clear();
		m_materials.clear();
		m_texture_2d_assets.clear();

		OAR_CORE_INFO("Scene unloaded");
	}



	SceneEntity& Scene::CreateEntity(const std::string& name, uint64_t uuid) {
		// Choose to create with uuid or not
		SceneEntity* ent = uuid == 0 ? new SceneEntity(this) : new SceneEntity(uuid, this);
		ent->name = name;
		m_entities.push_back(ent);

		// Give transform (all entities have this)
		m_transform_component_manager.AddComponent(ent);

		return *ent;

	}



	Material* Scene::CreateMaterial(uint64_t uuid) {
		Material* p_material = uuid == 0 ? new Material() : new Material(uuid);
		p_material->base_color_texture = GetTexture(DEFAULT_BASE_COLOR_TEX_ID);
		m_materials.push_back(p_material);
		return p_material;
	}



	void Scene::DeleteMaterial(uint64_t uuid) {
		auto it = std::ranges::find_if(m_materials, [uuid](auto* p_mat) {return p_mat->uuid() == uuid; });

		if (it == m_materials.end())
			return;

		m_mesh_component_manager.OnMaterialDeletion(*it, GetMaterial(BASE_MATERIAL_ID));
		delete* it;
		m_materials.erase(it);

	}

	Texture2D* Scene::CreateTexture2DAsset(const Texture2DSpec& spec, uint64_t uuid) {


		for (auto* p_texture : m_texture_2d_assets) {
			if (p_texture->m_spec.filepath == spec.filepath) {
				OAR_CORE_WARN("Texture '{0}' already created", spec.filepath);
				return p_texture;
			}
		}

		Texture2D* p_tex = uuid == 0 ? new Texture2D(spec.filepath.c_str()) : new Texture2D(spec.filepath.c_str(), uuid);
		m_texture_2d_assets.push_back(p_tex);

		p_tex->SetSpec(spec);
		p_tex->LoadFromFile();
		return p_tex;
	}

	MeshAsset* Scene::CreateMeshAsset(const std::string& filename, uint64_t uuid) {
		for (auto mesh_data : m_mesh_assets) {
			if (mesh_data->GetFilename() == filename) {
				OAR_CORE_WARN("Mesh asset already loaded in: {0}", filename);
				return mesh_data;
			}
		}

		MeshAsset* mesh_data = uuid == 0 ? new MeshAsset(filename) : new MeshAsset(filename, uuid);
		m_mesh_assets.push_back(mesh_data);
		return mesh_data;
	}



	void Scene::DeleteMeshAsset(MeshAsset* data) {
		m_mesh_assets.erase(std::find(m_mesh_assets.begin(), m_mesh_assets.end(), data));
		delete data;
	};



	void Scene::LoadMeshAssetIntoGPU(MeshAsset* asset) {


		if (asset->m_is_loaded) {
			OAR_CORE_ERROR("Mesh '{0}' is already loaded", asset->m_filename);
			return;
		}

		GL_StateManager::BindVAO(asset->m_vao);

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
			Texture2D* p_diffuse_texture = LoadMeshAssetTexture(dir, aiTextureType_BASE_COLOR, p_material);
			// Give the default diffuse texture if none is loaded with the material
			asset->m_original_materials[i].base_color_texture = p_diffuse_texture ? p_diffuse_texture : GetMaterial(BASE_MATERIAL_ID)->base_color_texture;
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
			if (p_material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == aiReturn_SUCCESS) {
				asset->m_original_materials[i].roughness = roughness;
			}

			float metallic;
			if (p_material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == aiReturn_SUCCESS) {
				asset->m_original_materials[i].metallic = metallic;
			}


			// Create material in scene so it can be used globally, and modified
			Material* p_added_material = CreateMaterial();

			// Link to scene materials so mesh components which have this asset can link to the default materials
			asset->m_scene_materials.push_back(p_added_material);

			// Make a copy to preserve original materials
			*p_added_material = asset->m_original_materials[i];
			p_added_material->name = std::format("{} - {}", asset->m_filename.substr(asset->m_filename.find_last_of("/") + 1), i);
		}

		// Load into gpu
		asset->PopulateBuffers();
		asset->importer.FreeScene();
		asset->m_is_loaded = true;

	}

	void Scene::LoadMeshAssetPreExistingMaterials(MeshAsset* asset, std::vector<Material*>& materials) {
		if (asset->m_is_loaded) {
			OAR_CORE_ERROR("Mesh '{0}' is already loaded", asset->m_filename);
			return;
		}
		GL_StateManager::BindVAO(asset->m_vao);
		asset->PopulateBuffers();
		asset->importer.FreeScene();
		asset->m_is_loaded = true;
		asset->m_scene_materials = materials;
	}


}