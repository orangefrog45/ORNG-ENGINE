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
#include "core/CodedAssets.h"
#include "rendering/EnvMapLoader.h"


namespace ORNG {


	Scene::~Scene() {
		UnloadScene();
	}

	Scene::Scene() {

	}


	void Scene::Update(float ts) {

		m_physics_system.OnUpdate(ts);

		m_mesh_component_manager.OnUpdate();
		m_pointlight_component_manager.OnUpdate();
		m_spotlight_component_manager.OnUpdate();
		m_camera_system.OnUpdate();

		//terrain.UpdateTerrainQuadtree(m_camera_system.p_active_camera->GetEntity()->GetComponent<TransformComponent>()->GetPosition());
	}




	void Scene::DeleteEntity(SceneEntity* p_entity) {
		auto it = std::ranges::find(m_entities, p_entity);

		auto& current_child_entity = p_entity->GetComponent<RelationshipComponent>()->first;
		while (current_child_entity != entt::null) {
			p_entity->mp_scene->DeleteEntity(GetEntity(current_child_entity));
		}

		delete p_entity;
		m_entities.erase(it);

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
		auto it = std::find_if(m_entities.begin(), m_entities.end(), [&](const auto* p_entity) {return p_entity->GetUUID() == uuid; });
		return it == m_entities.end() ? nullptr : *it;
	}
	SceneEntity* Scene::GetEntity(entt::entity handle) {
		auto it = std::find_if(m_entities.begin(), m_entities.end(), [&](const auto* p_entity) {return p_entity->m_entt_handle == handle; });
		return it == m_entities.end() ? nullptr : *it;
	}




	void Scene::LoadScene(const std::string& filepath) {
		TimeStep time = TimeStep(TimeStep::TimeUnits::MILLISECONDS);

		FastNoiseSIMD* noise = FastNoiseSIMD::NewFastNoiseSIMD();
		noise->SetFrequency(0.05f);
		noise->SetCellularReturnType(FastNoiseSIMD::Distance);
		noise->SetNoiseType(FastNoiseSIMD::PerlinFractal);

		Material* default_material = CreateMaterial(BASE_MATERIAL_ID);
		default_material->base_color_texture = &CodedAssets::GetBaseTexture();

		post_processing.global_fog.SetNoise(noise);
		terrain.Init(default_material);
		m_physics_system.OnLoad();
		m_mesh_component_manager.OnLoad();
		m_spotlight_component_manager.OnLoad();
		m_pointlight_component_manager.OnLoad();
		m_camera_system.OnLoad();

		if (!SceneSerializer::DeserializeScene(*this, filepath)) {
			EnvMapLoader::LoadEnvironmentMap("", skybox, 1);
		}
		ORNG_CORE_INFO("Scene loaded in {0}ms", time.GetTimeInterval());
	}




	void Scene::UnloadScene() {
		ORNG_CORE_INFO("Unloading scene...");

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

		m_registry.clear();
		m_physics_system.OnUnload();
		m_mesh_component_manager.OnUnload();
		m_spotlight_component_manager.OnUnload();
		m_pointlight_component_manager.OnUnload();
		m_camera_system.OnUnload();

		m_mesh_assets.clear();
		m_entities.clear();
		m_materials.clear();
		m_texture_2d_assets.clear();
		ORNG_CORE_INFO("Scene unloaded");
	}

	MeshAsset* Scene::GetMeshAsset(uint64_t uuid) {
		auto it = std::find_if(m_mesh_assets.begin(), m_mesh_assets.end(), [&](const auto* p_tex) {return p_tex->uuid() == uuid; });

		if (it == m_mesh_assets.end()) {
			ORNG_CORE_TRACE("Mesh with ID '{0}' does not exist, using CodedCube as replacement", uuid);
			return &CodedAssets::GetCubeAsset();
		}

		return *it;
	}

	SceneEntity& Scene::CreateEntity(const std::string& name, uint64_t uuid) {
		// Choose to create with uuid or not
		auto reg_ent = m_registry.create();
		SceneEntity* ent = uuid == 0 ? new SceneEntity(this, reg_ent) : new SceneEntity(uuid, reg_ent, this);
		ent->name = name;
		m_entities.push_back(ent);

		return *ent;

	}



	Material* Scene::CreateMaterial(uint64_t uuid) {
		Material* p_material = uuid == 0 ? new Material(&CodedAssets::GetBaseTexture()) : new Material(uuid);
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
				ORNG_CORE_WARN("Texture '{0}' already created", spec.filepath);
				return p_texture;
			}
		}

		Texture2D* p_tex = uuid == 0 ? new Texture2D(spec.filepath.c_str()) : new Texture2D(spec.filepath.c_str(), uuid);
		m_texture_2d_assets.push_back(p_tex);

		p_tex->SetSpec(spec);
		p_tex->LoadFromFile();
		return p_tex;
	}


	Texture2D* Scene::GetTexture(uint64_t uuid) {
		auto it = std::find_if(m_texture_2d_assets.begin(), m_texture_2d_assets.end(), [&](const auto* p_tex) {return p_tex->uuid() == uuid; });

		if (it == m_texture_2d_assets.end()) {
			ORNG_CORE_WARN("Texture with ID '{0}' does not exist, not found", uuid);
			return nullptr;
		}

		return *it;
	}


	Material* Scene::GetMaterial(uint64_t id) const {
		auto it = std::find_if(m_materials.begin(), m_materials.end(), [&](const auto* p_mat) {return p_mat->uuid() == id; });

		if (it == m_materials.end()) {
			ORNG_CORE_ERROR("Material with ID '{0}' does not exist, not found", id);
			return GetMaterial(BASE_MATERIAL_ID);
		}

		return *it;

	}

	MeshAsset* Scene::CreateMeshAsset(const std::string& filename, uint64_t uuid) {
		for (auto mesh_data : m_mesh_assets) {
			if (mesh_data->GetFilename() == filename) {
				ORNG_CORE_WARN("Mesh asset already loaded in: {0}", filename);
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
			ORNG_CORE_ERROR("Mesh '{0}' is already loaded", asset->m_filename);
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
			ORNG_CORE_ERROR("Mesh '{0}' is already loaded", asset->m_filename);
			return;
		}
		GL_StateManager::BindVAO(asset->m_vao);
		asset->PopulateBuffers();
		asset->importer.FreeScene();
		asset->m_is_loaded = true;
		asset->m_scene_materials = materials;
	}


}