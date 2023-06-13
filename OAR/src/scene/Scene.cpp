#include "pch/pch.h"

#include "util/TimeStep.h"
#include "scene/Scene.h"
#include "util/util.h"
#include "rendering/MeshInstanceGroup.h"
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
		m_mesh_instance_groups.reserve(20);
		m_mesh_components.reserve(100);


		Material* default_material = GetMaterial(CreateMaterial());
		default_material->name = "Default";
		default_material->base_color = glm::vec3(1);
		default_material->base_color_texture = CreateTexture2DAsset("./res/textures/missing_texture.jpeg");

	}


	void Scene::Update() {
		for (int i = 0; i < m_mesh_instance_groups.size(); i++) {
			auto* group = m_mesh_instance_groups[i];

			group->ProcessUpdates();

			// Check if group should be deleted
			if (group->m_instances.empty()) {
				delete group;
				m_mesh_instance_groups.erase(m_mesh_instance_groups.begin() + i);
			}
		}
		for (auto& script : m_script_components) {
			if (!script || !script->OnUpdate)
				continue;
			else
				script->OnUpdate();
		}

		if (mp_active_camera)
			mp_active_camera->Update();

		m_terrain.UpdateTerrainQuadtree(mp_active_camera->pos);
	}

	CameraComponent* Scene::AddCameraComponent(unsigned long entity_id) {
		if (GetComponent<CameraComponent>(entity_id)) {
			OAR_CORE_WARN("Camera component not added, entity '{0}' already has a camera component", m_entities[entity_id]->name);
			return nullptr;
		}

		CameraComponent* comp = new CameraComponent(entity_id);
		m_camera_components.push_back(comp);
		return comp;

	}


	PointLightComponent* Scene::AddPointLightComponent(unsigned long entity_id) {
		if (GetComponent<PointLightComponent>(entity_id)) {
			OAR_CORE_WARN("Pointlight component not added, entity '{0}' already has a pointlight component", m_entities[entity_id]->name);
			return nullptr;
		}

		PointLightComponent* comp = new PointLightComponent(entity_id);
		m_point_lights.push_back(comp);
		return comp;
	}

	SpotLightComponent* Scene::AddSpotLightComponent(unsigned long entity_id) {
		if (GetComponent<SpotLightComponent>(entity_id)) {
			OAR_CORE_WARN("Spotlight component not added, entity '{0}' already has a spotlight component", m_entities[entity_id]->name);
			return nullptr;
		}
		SpotLightComponent* comp = new SpotLightComponent(entity_id);
		m_spot_lights.push_back(comp);
		return comp;
	}

	ScriptComponent* Scene::AddScriptComponent(unsigned long entity_id) {
		if (GetComponent<ScriptComponent>(entity_id)) {
			OAR_CORE_WARN("Script component not added, entity '{0}' already has a script component", m_entities[entity_id]->name);
			return nullptr;
		}

		auto* p_comp = new ScriptComponent(entity_id);
		m_script_components.push_back(p_comp);
		return p_comp;
	}

	MeshComponent* Scene::AddMeshComponent(unsigned long entity_id, const std::string& filename, unsigned int shader_id) {

		// Currently any mesh components inside scene are auto-instanced and grouped together in MeshInstanceGroups, depending on shader, materials and asset
		if (GetComponent<MeshComponent>(entity_id)) {
			OAR_CORE_WARN("Mesh component not added, entity '{0}' already has a mesh component", m_entities[entity_id]->name);
			return nullptr;
		}


		MeshComponent* comp = new MeshComponent(entity_id);

		//check if mesh data is already available for the instance group about to be created if new component does not have appropiate instance group
		int mesh_asset_index = -1;
		for (int i = 0; i < m_mesh_assets.size(); i++) {
			if (m_mesh_assets[i]->GetFilename() == filename) {
				mesh_asset_index = i;
				break;
			}
		}

		comp->m_shader_id = shader_id;

		if (mesh_asset_index == -1) {

			MeshAsset* mesh_data = CreateMeshAsset(filename);
			auto group = new MeshInstanceGroup(mesh_data, comp->m_shader_id, this, comp->m_materials);

			m_mesh_instance_groups.push_back(group);
			group->AddMeshPtr(comp);
		}
		else {
			SortMeshIntoInstanceGroup(comp, m_mesh_assets[mesh_asset_index]);
		}

		m_mesh_components.push_back(comp);
		return comp;
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
				p_tex = CreateTexture2DAsset(full_path);

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


		FastNoiseSIMD* noise = FastNoiseSIMD::NewFastNoiseSIMD();
		noise->SetFrequency(0.05f);
		noise->SetCellularReturnType(FastNoiseSIMD::Distance);
		noise->SetNoiseType(FastNoiseSIMD::PerlinFractal);
		m_global_fog.SetNoise(noise);



		Material* p_terrain_material = GetMaterial(CreateMaterial());
		p_terrain_material->name = "Terrain";
		p_terrain_material->base_color = glm::vec3(1);
		p_terrain_material->base_color_texture = GetMaterial(0)->base_color_texture;
		p_terrain_material->roughness_texture = CreateTexture2DAsset("./res/textures/rock/strata-rock1_roughness.png.");
		p_terrain_material->ao_texture = CreateTexture2DAsset("./res/textures/rock/strata-rock1_ao.png");
		p_terrain_material->metallic_texture = CreateTexture2DAsset("./res/textures/rock/strata-rock1_metallic.png");
		p_terrain_material->displacement_texture = CreateTexture2DAsset("./res/textures/rock/strata-rock1_height.png");
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
		for (auto& group : m_mesh_instance_groups) {
			//Set materials
			for (auto* p_material : group->m_mesh_asset->m_scene_materials) {
				group->m_materials.push_back(p_material);
			}

			for (auto* p_mesh : group->m_instances) {
				p_mesh->m_materials = group->m_materials;
			}
			group->UpdateTransformSSBO();
		}

		OAR_CORE_INFO("Scene loaded in {0}ms", time.GetTimeInterval());
	}

	void Scene::UnloadScene() {
		OAR_CORE_INFO("Unloading scene...");
		for (MeshAsset* mesh_data : m_mesh_assets) {
			delete mesh_data;
		}
		for (auto* mesh : m_mesh_components) {
			delete mesh;
		}
		for (auto* group : m_mesh_instance_groups) {
			delete group;
		}
		for (auto* light : m_point_lights) {
			delete light;
		}
		for (auto* light : m_spot_lights) {
			delete light;
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

		OAR_CORE_INFO("Scene unloaded");
	}

	void Scene::SortMeshIntoInstanceGroup(MeshComponent* comp, MeshAsset* asset) {

		// this tells the instance groups to update the transform buffers every frame before rendering to set the correct transforms, as now multiple sets of transforms are needed
		asset->m_is_shared_in_instance_groups = true;

		int group_index = -1;

		// check if new entity can merge into already existing instance group
		for (int i = 0; i < m_mesh_instance_groups.size(); i++) {
			//if same data, shader and material, can be combined so instancing is possible
			if (m_mesh_instance_groups[i]->m_mesh_asset == asset
				&& m_mesh_instance_groups[i]->m_group_shader_id == comp->m_shader_id
				&& m_mesh_instance_groups[i]->m_materials == comp->m_materials) {
				group_index = i;
				break;
			}
		}

		if (group_index != -1) { // if instance group exists, place into
			// add mesh component's world transform into instance group for instanced rendering
			MeshInstanceGroup* group = m_mesh_instance_groups[group_index];
			group->AddMeshPtr(comp);
		}
		else { //else if instance group doesn't exist but mesh data exists, create group with existing data
			MeshInstanceGroup* group = new MeshInstanceGroup(asset, comp->m_shader_id, this, comp->m_materials);
			m_mesh_instance_groups.push_back(group);
			group->AddMeshPtr(comp);
		}

	}

	SceneEntity& Scene::CreateEntity(const char* name) {
		SceneEntity* ent = new SceneEntity(CreateEntityID(), this);
		ent->name = name;
		m_entities.push_back(ent);

		return *ent;

	}

	unsigned int Scene::CreateMaterial() {
		Material* p_material = new Material();

		//ID of material is its position in the material vector for quick material lookups in shaders.

		m_materials.push_back(p_material);
		p_material->material_id = m_materials.size() - 1;

		Renderer::GetShaderLibrary().UpdateMaterialUBO(m_materials);
		return p_material->material_id;
	}

	Texture2D* Scene::CreateTexture2DAsset(const std::string& filename) {

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

		// Remove asset from all components using it
		for (int i = 0; i < m_mesh_instance_groups.size(); i++) {
			MeshInstanceGroup* group = m_mesh_instance_groups[i];

			if (group->m_mesh_asset == data) {
				group->ClearMeshes();
				for (auto& mesh : group->m_instances) {
					mesh->mp_mesh_asset = nullptr;
				}

				// Delete all mesh instance groups using the asset as they cannot function without it
				m_mesh_instance_groups.erase(m_mesh_instance_groups.begin() + i);
				delete group;
			}
		}
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
			unsigned int new_material_id = CreateMaterial();
			Material* p_added_material = GetMaterial(new_material_id);

			// Link to scene materials so mesh components which have this asset can link to the default materials
			asset->m_scene_materials.push_back(p_added_material);

			// Make a copy to preserve original materials
			*p_added_material = asset->m_original_materials[i];
			p_added_material->name = std::format("{} - {}", asset->m_filename.substr(asset->m_filename.find_last_of("/") + 1), i);
			p_added_material->material_id = new_material_id;
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