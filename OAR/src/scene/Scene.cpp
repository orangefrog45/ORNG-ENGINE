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


namespace ORNG {

	Scene::~Scene() {
		UnloadScene();
	}

	Scene::Scene() {

		Material* default_material = CreateMaterial();
		default_material->name = "Default";
		default_material->base_color = glm::vec3(1);
		default_material->base_color_texture = CreateTexture2DAsset("./res/textures/missing_texture.png", true);

	}


	void Scene::Update() {

		for (auto* script : m_script_components) {
			//script->OnUpdate();
		}

		m_mesh_component_manager.OnUpdate();
		m_pointlight_component_manager.OnUpdate();
		m_spotlight_component_manager.OnUpdate();
		m_transform_component_manager.OnUpdate();


		if (mp_active_camera)
			mp_active_camera->Update();

		m_terrain.UpdateTerrainQuadtree(mp_active_camera->pos);
	}






	CameraComponent* Scene::AddCameraComponent(SceneEntity* p_entity) {
		if (GetComponent<CameraComponent>(p_entity->GetID())) {
			OAR_CORE_WARN("Camera component not added, entity '{0}' already has a camera component", p_entity->name);
			return nullptr;
		}

		CameraComponent* comp = new CameraComponent(p_entity);
		m_camera_components.push_back(comp);
		return comp;

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
				if (type == aiTextureType_BASE_COLOR)
					p_tex = CreateTexture2DAsset(full_path, true);
				else
					p_tex = CreateTexture2DAsset(full_path, false);


			}

		}

		return p_tex;
	}



	SceneEntity* Scene::GetEntity(unsigned long id) {
		auto it = std::find_if(m_entities.begin(), m_entities.end(), [&](const auto* p_entity) {return p_entity->GetID() == id; });
		return it == m_entities.end() ? nullptr : *it;
	}




	void Scene::LoadScene() {
		TimeStep time = TimeStep(TimeStep::TimeUnits::MILLISECONDS);
		m_skybox.Init();
		m_skybox.LoadEnvironmentMap("./res/textures/belfast_sunset_puresky_4k.hdr");


		FastNoiseSIMD* noise = FastNoiseSIMD::NewFastNoiseSIMD();
		noise->SetFrequency(0.05f);
		noise->SetCellularReturnType(FastNoiseSIMD::Distance);
		noise->SetNoiseType(FastNoiseSIMD::PerlinFractal);
		m_global_fog.SetNoise(noise);



		Material* p_terrain_material = CreateMaterial();
		p_terrain_material->name = "Terrain";
		p_terrain_material->base_color = glm::vec3(1);
		p_terrain_material->base_color_texture = GetMaterial(0)->base_color_texture;
		p_terrain_material->roughness_texture = CreateTexture2DAsset("./res/textures/rock/strata-rock1_roughness.png.", false);
		p_terrain_material->ao_texture = CreateTexture2DAsset("./res/textures/rock/strata-rock1_ao.png", false);
		p_terrain_material->metallic_texture = CreateTexture2DAsset("./res/textures/rock/strata-rock1_metallic.png", false);
		m_terrain.Init(p_terrain_material->material_id);

		for (auto& mesh : m_mesh_assets) {
			if (mesh->GetLoadStatus() == false) {
				m_futures.push_back(std::async(std::launch::async, [&mesh] {mesh->LoadMeshData(); }));
			}
		}
		for (unsigned int i = 0; i < m_futures.size(); i++) {
			m_futures[i].get();
		}
		for (auto& mesh : m_mesh_assets) {
			if (mesh->GetLoadStatus() == false) {
				LoadMeshAssetIntoGPU(mesh);
			}
		}


		m_mesh_component_manager.OnLoad();
		m_spotlight_component_manager.OnLoad();
		m_pointlight_component_manager.OnLoad();

		OAR_CORE_INFO("Scene loaded in {0}ms", time.GetTimeInterval());
	}

	void Scene::UnloadScene() {
		OAR_CORE_INFO("Unloading scene...");
		for (MeshAsset* mesh_data : m_mesh_assets) {
			delete mesh_data;
		}
		for (auto* entity : m_entities) {
			delete entity;
		}
		for (auto* script : m_script_components) {
			delete script;
		}
		for (auto* texture : m_texture_2d_assets) {
			if (texture)
				delete texture;
		}
		for (auto* material : m_materials) {
			delete material;
		}

		m_mesh_component_manager.OnUnload();
		m_spotlight_component_manager.OnUnload();
		m_pointlight_component_manager.OnUnload();
		m_transform_component_manager.OnUnload();

		OAR_CORE_INFO("Scene unloaded");
	}



	SceneEntity& Scene::CreateEntity(const char* name) {
		SceneEntity* ent = new SceneEntity(CreateEntityID(), this);
		ent->name = name;
		m_entities.push_back(ent);

		// Give transform (all entities have this)
		m_transform_component_manager.AddComponent(ent);

		return *ent;

	}

	Material* Scene::CreateMaterial() {
		Material* p_material = new Material();

		//ID of material is its position in the material vector for quick material lookups in shaders.

		m_materials.push_back(p_material);
		p_material->material_id = m_materials.size() - 1;

		Renderer::GetShaderLibrary().UpdateMaterialUBO(m_materials);
		return p_material;
	}

	Texture2D* Scene::CreateTexture2DAsset(const std::string& filename, bool srgb) {

		static Texture2DSpec base_spec;
		base_spec.mag_filter = GL_LINEAR;
		base_spec.min_filter = GL_LINEAR;

		for (auto& p_texture : m_texture_2d_assets) {
			if (p_texture->m_spec.filepath == filename) {
				OAR_CORE_WARN("Texture '{0}' already created", filename);
				return nullptr;
			}
		}

		Texture2D* p_tex = new Texture2D(filename.c_str());
		base_spec.filepath = filename;
		base_spec.srgb_space = srgb;
		m_texture_2d_assets.push_back(p_tex);

		p_tex->SetSpec(base_spec);
		p_tex->LoadFromFile();
		return p_tex;
	}

	MeshAsset* Scene::CreateMeshAsset(const std::string& filename) {
		for (auto mesh_data : m_mesh_assets) {
			if (mesh_data->GetFilename() == filename) {
				OAR_CORE_WARN("Mesh asset already loaded in: {0}", filename);
				return mesh_data;
			}
		}

		MeshAsset* mesh_data = new MeshAsset(filename);
		m_mesh_assets.push_back(mesh_data);
		return mesh_data;
	}



	void Scene::DeleteMeshAsset(MeshAsset* data) {
		m_mesh_assets.erase(std::find(m_mesh_assets.begin(), m_mesh_assets.end(), data));
		delete data;
	};



	void Scene::LoadMeshAssetIntoGPU(MeshAsset* asset) {


		if (asset->is_loaded) {
			// Shouldn't be possible
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
			asset->m_original_materials[i].base_color_texture = p_diffuse_texture ? p_diffuse_texture : GetMaterial(0)->base_color_texture;
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
				asset->m_original_materials[i].roughness = metallic;
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
		asset->is_loaded = true;

	}

	void Scene::MakeCameraActive(CameraComponent* p_cam) {
		mp_active_camera = p_cam;
		p_cam->is_active = true;

		for (auto* cam : m_camera_components) {
			if (cam && p_cam != cam)
				cam->is_active = false;
		}
	}

}