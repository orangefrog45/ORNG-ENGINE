#include "pch/pch.h"
#include "assets/AssetManager.h"
#include "assets/PhysXMaterialAsset.h"
#include "events/EventManager.h"
#include "rendering/Textures.h"
#include "rendering/MeshAsset.h"
#include "core/GLStateManager.h"
#include "physics/Physics.h" // for material initialization
#include "rendering/EnvMapLoader.h"


namespace ORNG {
	void AssetManager::I_Init() {
		InitBaseAssets();
		serializer.Init();

		// Each frame, check if any meshes have finished loading vertex data and load them into GPU if they have
		m_update_listener.OnEvent = [this](const Events::EngineCoreEvent& t_event) {
			if (t_event.event_type != Events::EngineCoreEvent::EventType::UPDATE) return;
			serializer.ProcessAssetQueues();
			};

		Events::EventManager::RegisterListener(m_update_listener);
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


	void AssetManager::OnTextureDelete(Texture2D* p_tex) {
		// If any materials use this texture, remove it from them
		for (auto& [key, p_asset] : Get().m_assets) {
			auto* p_material = dynamic_cast<Material*>(p_asset);
			if (!p_material)
				continue;

			p_material->base_colour_texture = p_material->base_colour_texture == p_tex ? nullptr : p_material->base_colour_texture;
			p_material->normal_map_texture = p_material->normal_map_texture == p_tex ? nullptr : p_material->normal_map_texture;
			p_material->emissive_texture = p_material->emissive_texture == p_tex ? nullptr : p_material->emissive_texture;
			p_material->displacement_texture = p_material->displacement_texture == p_tex ? nullptr : p_material->displacement_texture;
			p_material->metallic_texture = p_material->metallic_texture == p_tex ? nullptr : p_material->metallic_texture;
			p_material->roughness_texture = p_material->roughness_texture == p_tex ? nullptr : p_material->roughness_texture;
		}
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
			ScriptingEngine::UnloadScriptDLL("res/scripts/src/" + p_script->symbols.script_name + ".cpp");
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
		mp_base_sphere = std::make_unique<MeshAsset>("res/meshes/Sphere.glb");
		serializer.DeserializeAssetBinary("res/core-res/meshes/Sphere.glb.omesh", *mp_base_sphere);
		mp_base_sphere->m_vao.FillBuffers();
		mp_base_sphere->m_is_loaded = true;
		mp_base_sphere->uuid = UUID<uint64_t>{ ORNG_BASE_SPHERE_ID };
		mp_base_sphere->m_material_uuids.push_back(ORNG_BASE_MATERIAL_ID);
		AddAsset(&*mp_base_sphere);

		m_assets.erase(ORNG_BASE_CUBE_ID);
		mp_base_cube.release();
		mp_base_cube = std::make_unique<MeshAsset>("res/meshes/cube.glb");
		serializer.DeserializeAssetBinary("res/core-res/meshes/cube.glb.omesh", *mp_base_cube);
		mp_base_cube->m_vao.FillBuffers();
		mp_base_cube->m_is_loaded = true;
		mp_base_cube->uuid = UUID<uint64_t>{ ORNG_BASE_CUBE_ID };
		mp_base_cube->m_material_uuids.push_back(ORNG_BASE_MATERIAL_ID);
		AddAsset(&*mp_base_cube);
	}

	void AssetManager::InitBaseAssets() {
		InitBaseTexture();
		InitBase3DQuad();
		
		LoadExternalBaseAssets("");

		mp_base_material = std::make_unique<Material>((uint64_t)ORNG_BASE_MATERIAL_ID);
		mp_base_material->name = "Base material";

		auto symbols = ScriptSymbols("");
		mp_base_script = std::make_unique<ScriptAsset>("", symbols);
		mp_base_script->uuid = UUID<uint64_t>(ORNG_BASE_SCRIPT_ID);
		mp_base_tex->uuid = UUID<uint64_t>(ORNG_BASE_TEX_ID);
		mp_base_material->uuid = UUID<uint64_t>(ORNG_BASE_MATERIAL_ID);
		mp_base_brdf_lut = std::make_unique<Texture2D>("Base BRDF LUT");
		mp_base_brdf_lut->uuid = UUID<uint64_t>(ORNG_BASE_BRDF_LUT_ID);

		EnvMapLoader loader{};
		loader.LoadBRDFConvolution(*mp_base_brdf_lut);

		AddAsset(&*mp_base_tex);
		AddAsset(&*mp_base_material);
		AddAsset(&*mp_base_script);
		AddAsset(&*mp_base_quad);

		if (bool physics_module_active = Physics::GetPhysics()) {
			auto* p_phys_mat = new PhysXMaterialAsset("BASE");
			p_phys_mat->uuid = UUID<uint64_t>(ORNG_BASE_PHYSX_MATERIAL_ID);
			p_phys_mat->p_material = Physics::GetPhysics()->createMaterial(0.75f, 0.75f, 0.6f);
			AddAsset(&*p_phys_mat);
		}

	}

	void AssetManager::IOnShutdown() {
		ClearAll();
		auto& instance = Get();

		instance.mp_base_material.release();
		instance.mp_base_sound.release();
		instance.mp_base_tex.release();
		instance.mp_base_cube.release();
		instance.mp_base_sphere.release();
		instance.mp_base_quad.release();
		instance.mp_base_script.release();

		if (bool physics_module_active = Physics::GetPhysics())
			DeleteAsset(ORNG_BASE_PHYSX_MATERIAL_ID);
	};

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