#include "pch/pch.h"
#include "events/Events.h"
#include "components/ComponentAPI.h"
#include "scene/SceneSerializer.h"
#include "scene/Scene.h"
#include "components/ComponentSystems.h"
#include "scene/SceneEntity.h"
#include "assets/AssetManager.h"
#include "util/InterpolatorSerializer.h"
#include "assets/Prefab.h"
#include "scene/SerializationUtil.h"

namespace ORNG {
	std::string SceneSerializer::SerializeEntityIntoString(SceneEntity& entity) {
		YAML::Emitter out;

		SerializeEntity(entity, out);
		return std::string{ out.c_str() };
	}

	std::string SceneSerializer::SerializeEntityArrayIntoString(const std::vector<SceneEntity*>& entities) {
		YAML::Emitter out;

		out << YAML::BeginMap;
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		for (auto* p_ent : entities) {
			SerializeEntity(*p_ent, out);
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		return out.c_str();
	}

	void SceneSerializer::RemapEntityReferences(const std::unordered_map<uint64_t, uint64_t>& uuid_lookup, const std::vector<SceneEntity*>& entities) {
		for (auto* p_ent : entities) {
			Events::EventManager::DispatchEvent(EntitySerializationEvent{p_ent, &uuid_lookup});
		}
	}

	SceneEntity& SceneSerializer::DeserializeEntityUUIDFromString(Scene& scene, const std::string& str) {
		YAML::Node data = YAML::Load(str);
		//Create entities in first pass so they can be linked as parent/children in 2nd pass
		return scene.CreateEntity(data["Name"].as<std::string>(), data["Entity"].as<uint64_t>());
	}

	std::vector<SceneEntity*> SceneSerializer::DeserializePrefab(Scene& scene, const Prefab& prefab) {
		ORNG_TRACY_PROFILE;

		const YAML::Node& data = prefab.node;

		std::vector<SceneEntity*> ents;

		// key = serialized uuid, val = instantiation uuid
		std::unordered_map<uint64_t, uint64_t> id_mappings;

		auto entities = data["Entities"];

		//Create entities in first pass so they can be linked as parent/children in 2nd pass
		for (auto entity_node : entities) {
			auto* p_ent = &scene.CreateEntity(entity_node["Name"].as<std::string>());
			ents.push_back(p_ent);

			// Store serialized : new uuid pairs to link parents/children properly
			id_mappings[entity_node["Entity"].as<uint64_t>()] = p_ent->GetUUID();
		}

		for (auto entity_node : entities) {
			auto* p_ent = scene.GetEntity(id_mappings[entity_node["Entity"].as<uint64_t>()]);
			uint64_t serialized_parent_uuid = entity_node["ParentID"].as<uint64_t>();

			if (serialized_parent_uuid != 0) {
				auto* p_parent = scene.GetEntity(id_mappings[serialized_parent_uuid]);
				ASSERT(p_parent);
				p_ent->SetParent(*p_parent);
			}

			DeserializeEntity(scene, entity_node, *p_ent);
		}

		RemapEntityReferences(id_mappings, ents);

		return ents;
	}


	void SceneSerializer::DeserializeEntityFromString(Scene& scene, const std::string& str, SceneEntity& entity, bool ignore_parent) {
		YAML::Node data = YAML::Load(str);
		DeserializeEntity(scene, data, entity, ignore_parent);
	}


	void SceneSerializer::SerializeEntity(SceneEntity& entity, YAML::Emitter& out) {
		out << YAML::BeginMap;
		out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();
		out << YAML::Key << "Name" << YAML::Value << entity.name;
		auto* p_parent = entity.GetScene()->GetEntity(entity.GetParent());
		out << YAML::Key << "ParentID" << YAML::Value << (p_parent ? p_parent->GetUUID() : 0);

		const auto* p_transform = entity.GetComponent<TransformComponent>();

		out << YAML::Key << "TransformComp";
		out << YAML::BeginMap;
		out << YAML::Key << "Pos" << YAML::Value << p_transform->GetPosition();
		out << YAML::Key << "Scale" << YAML::Value << p_transform->GetScale();
		out << YAML::Key << "Orientation" << YAML::Value << p_transform->GetOrientation();

		out << YAML::Key << "Absolute" << YAML::Value << p_transform->m_is_absolute;

		out << YAML::EndMap;

		auto* p_mesh_comp = entity.GetComponent<MeshComponent>();

		if (p_mesh_comp) {
			out << YAML::Key << "MeshComp";
			out << YAML::BeginMap;
			out << YAML::Key << "MeshAssetID" << YAML::Value << p_mesh_comp->GetMeshData()->uuid();

			out << YAML::Key << "Materials" << YAML::Value;
			out << YAML::Flow;
			out << YAML::BeginSeq;
			for (auto* p_material : p_mesh_comp->GetMaterials()) {
				out << p_material->uuid();
			}
			out << YAML::EndSeq;
			out << YAML::EndMap;
		}


		const auto* p_pointlight = entity.GetComponent<PointLightComponent>();

		if (p_pointlight) {
			out << YAML::Key << "PointlightComp";
			out << YAML::BeginMap;

			out << YAML::Key << "Colour" << YAML::Value << p_pointlight->colour;
			out << YAML::Key << "AttenConstant" << YAML::Value << p_pointlight->attenuation.constant;
			out << YAML::Key << "AttenLinear" << YAML::Value << p_pointlight->attenuation.linear;
			out << YAML::Key << "AttenExp" << YAML::Value << p_pointlight->attenuation.exp;
			out << YAML::Key << "Shadows" << YAML::Value << p_pointlight->shadows_enabled;
			out << YAML::Key << "ShadowDistance" << YAML::Value << p_pointlight->shadow_distance;

			out << YAML::EndMap;
		}

		const auto* p_spotlight = entity.GetComponent<SpotLightComponent>();

		if (p_spotlight) {
			out << YAML::Key << "SpotlightComp";
			out << YAML::BeginMap;

			out << YAML::Key << "Colour" << YAML::Value << p_spotlight->colour;
			out << YAML::Key << "AttenConstant" << YAML::Value << p_spotlight->attenuation.constant;
			out << YAML::Key << "AttenLinear" << YAML::Value << p_spotlight->attenuation.linear;
			out << YAML::Key << "AttenExp" << YAML::Value << p_spotlight->attenuation.exp;
			out << YAML::Key << "Aperture" << YAML::Value << p_spotlight->m_aperture;
			out << YAML::Key << "Shadows" << YAML::Value << p_spotlight->shadows_enabled;
			out << YAML::Key << "ShadowDistance" << YAML::Value << p_spotlight->shadow_distance;

			out << YAML::EndMap;
		}

		const auto* p_cam = entity.GetComponent<CameraComponent>();

		if (p_cam) {
			out << YAML::Key << "CameraComp";
			out << YAML::BeginMap;

			out << YAML::Key << "zNear" << YAML::Value << p_cam->zNear;
			out << YAML::Key << "zFar" << YAML::Value << p_cam->zFar;
			out << YAML::Key << "FOV" << YAML::Value << p_cam->fov;
			out << YAML::Key << "Exposure" << YAML::Value << p_cam->exposure;

			out << YAML::EndMap;
		}

		ScriptComponent* p_script_comp = entity.GetComponent<ScriptComponent>();
		if (p_script_comp) {
			out << YAML::Key << "ScriptComp" << YAML::BeginMap;

			if (p_script_comp->GetSymbols())
				out << YAML::Key << "ScriptName" << YAML::Value << (p_script_comp->GetSymbols()->loaded ? p_script_comp->GetSymbols()->script_name : "");
			else 
				out << YAML::Key << "ScriptName" << YAML::Value << "";

			out << YAML::EndMap;
		}

		AudioComponent* p_audio_comp = entity.GetComponent<AudioComponent>();
		if (p_audio_comp) {
			out << YAML::Key << "AudioComp" << YAML::BeginMap;
			out << YAML::Key << "Volume" << YAML::Value << p_audio_comp->m_volume;
			out << YAML::Key << "Pitch" << YAML::Value << p_audio_comp->m_pitch;
			out << YAML::Key << "AudioUUID" << YAML::Value << p_audio_comp->m_sound_asset_uuid;
			out << YAML::Key << "MinRange" << YAML::Value << p_audio_comp->m_range.min;
			out << YAML::Key << "MaxRange" << YAML::Value << p_audio_comp->m_range.max;
			out << YAML::EndMap;
		}

		if (auto* p_emitter = entity.GetComponent<ParticleEmitterComponent>()) {
			Out(out, "ParticleEmitterComp", YAML::BeginMap);
			Out(out, "Spread", p_emitter->GetSpread());
			Out(out, "Spawn extents", p_emitter->GetSpawnExtents());
			Out(out, "Velocity range", p_emitter->GetVelocityScale());
			Out(out, "Nb. particles", p_emitter->GetNbParticles());
			Out(out, "Lifespan", p_emitter->GetParticleLifespan());
			Out(out, "Spawn delay", p_emitter->GetSpawnDelay());
			Out(out, "Type", (unsigned)p_emitter->GetType());
			Out(out, "Acceleration", p_emitter->GetAcceleration());
			Out(out, "Active", p_emitter->IsActive());

			InterpolatorSerializer::SerializeInterpolator("Colour over time", out, p_emitter->m_life_colour_interpolator);
			InterpolatorSerializer::SerializeInterpolator("Alpha over time", out, p_emitter->m_life_alpha_interpolator);
			InterpolatorSerializer::SerializeInterpolator("Scale over time", out, p_emitter->m_life_scale_interpolator);

			if (p_emitter->GetType() == ParticleEmitterComponent::BILLBOARD) {
				Out(out, "MaterialUUID", entity.GetComponent<ParticleBillboardResources>()->p_material->uuid());
			}
			else {
				auto* p_res = entity.GetComponent<ParticleMeshResources>();

				Out(out, "MeshUUID", p_res->p_mesh->uuid());

				Out(out, "Materials", YAML::Flow);
				out << YAML::BeginSeq;
				for (auto* p_material : p_res->materials) {
					out << p_material->uuid();
				}
				out << YAML::EndSeq;
			}

			out << YAML::EndMap;
		}


		if (auto* p_buffer = entity.GetComponent<ParticleBufferComponent>()) {
			Out(out, "ParticleBufferComp", YAML::BeginMap);
			Out(out, "BufferID", p_buffer->GetBufferID());
			Out(out, "Min allocated particles", p_buffer->GetMinAllocatedParticles());

			out << YAML::EndMap;
		}

		Events::EventManager::DispatchEvent(EntitySerializationEvent{&entity, &out});

		out << YAML::EndMap;
	}

	void SceneSerializer::DeserializePointlightComp(const YAML::Node& light_node, SceneEntity& entity) {
		auto* p_pointlight_comp = entity.AddComponent<PointLightComponent>();
		p_pointlight_comp->colour = light_node["Colour"].as<glm::vec3>();
		p_pointlight_comp->attenuation.constant = light_node["AttenConstant"].as<float>();
		p_pointlight_comp->attenuation.linear = light_node["AttenLinear"].as<float>();
		p_pointlight_comp->attenuation.exp = light_node["AttenExp"].as<float>();
		p_pointlight_comp->shadows_enabled = light_node["Shadows"].as<bool>();
		p_pointlight_comp->shadow_distance = light_node["ShadowDistance"].as<float>();
	}

	void SceneSerializer::DeserializeSpotlightComp(const YAML::Node& light_node, SceneEntity& entity) {
		auto* p_spotlight_comp = entity.AddComponent<SpotLightComponent>();
		p_spotlight_comp->colour = light_node["Colour"].as<glm::vec3>();
		p_spotlight_comp->attenuation.constant = light_node["AttenConstant"].as<float>();
		p_spotlight_comp->attenuation.linear = light_node["AttenLinear"].as<float>();
		p_spotlight_comp->attenuation.exp = light_node["AttenExp"].as<float>();
		p_spotlight_comp->m_aperture = light_node["Aperture"].as<float>();
		p_spotlight_comp->shadows_enabled = light_node["Shadows"].as<bool>();
		p_spotlight_comp->shadow_distance = light_node["ShadowDistance"].as<float>();
	}

	void SceneSerializer::DeserializeCameraComp(const YAML::Node& node, SceneEntity& entity) {
		auto* p_cam_comp = entity.AddComponent<CameraComponent>();
		p_cam_comp->fov = node["FOV"].as<float>();
		p_cam_comp->exposure = node["Exposure"].as<float>();
		p_cam_comp->zFar = node["zFar"].as<float>();
		p_cam_comp->zNear = node["zNear"].as<float>();
	}

	void SceneSerializer::DeserializeScriptComp(const YAML::Node& node, SceneEntity& entity) {
		auto* p_script_comp = entity.AddComponent<ScriptComponent>();
		std::string script_name = node["ScriptName"].as<std::string>();

		auto* p_asset = AssetManager::GetAsset<ScriptAsset>("res/scripts/src/" + script_name + ".cpp");

		if (p_asset) {
			p_script_comp->SetSymbols(&p_asset->symbols);
		}
		else {
			ORNG_CORE_ERROR("Scene deserialization error: no script file with name '{0}' found", script_name);
			p_asset = AssetManager::GetAsset<ScriptAsset>(ORNG_BASE_SCRIPT_ID);
			p_script_comp->SetSymbols(&p_asset->symbols);
		}
	}

	void SceneSerializer::DeserializeAudioComp(const YAML::Node& node, SceneEntity& entity) {
		AudioComponent* p_audio = entity.AddComponent<AudioComponent>();
		p_audio->SetVolume(node["Volume"].as<float>());
		p_audio->SetPitch(node["Pitch"].as<float>());
		p_audio->SetSoundAssetUUID(node["AudioUUID"].as<uint64_t>());
		p_audio->SetMinMaxRange(node["MinRange"].as<float>(), node["MaxRange"].as<float>());
	}

	void SceneSerializer::DeserializeParticleEmitterComp(const YAML::Node& emitter_node, SceneEntity& entity) {
		auto* p_emitter = entity.AddComponent<ParticleEmitterComponent>();
		p_emitter->m_spread = emitter_node["Spread"].as<float>();
		p_emitter->m_spawn_extents = emitter_node["Spawn extents"].as<glm::vec3>();
		p_emitter->m_velocity_min_max_scalar = emitter_node["Velocity range"].as<glm::vec2>();
		p_emitter->m_num_particles = emitter_node["Nb. particles"].as<unsigned>();
		p_emitter->m_particle_lifespan_ms = emitter_node["Lifespan"].as<float>();
		p_emitter->m_particle_spawn_delay_ms = emitter_node["Spawn delay"].as<float>();
		p_emitter->acceleration = emitter_node["Acceleration"].as<glm::vec3>();
		p_emitter->m_active = emitter_node["Active"].as<bool>();
		p_emitter->SetType(static_cast<ParticleEmitterComponent::EmitterType>(emitter_node["Type"].as<unsigned>()));

		InterpolatorSerializer::DeserializeInterpolator(emitter_node["Alpha over time"], p_emitter->m_life_alpha_interpolator);
		InterpolatorSerializer::DeserializeInterpolator(emitter_node["Colour over time"], p_emitter->m_life_colour_interpolator);
		InterpolatorSerializer::DeserializeInterpolator(emitter_node["Scale over time"], p_emitter->m_life_scale_interpolator);
		if (p_emitter->GetType() == ParticleEmitterComponent::BILLBOARD) {
			auto* p_mat = AssetManager::GetAsset<Material>(emitter_node["MaterialUUID"].as<uint64_t>());
			entity.GetComponent<ParticleBillboardResources>()->p_material = p_mat ? p_mat : AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID);
		}
		else {
			auto* p_res = entity.GetComponent<ParticleMeshResources>();
			p_res->p_mesh = AssetManager::GetAsset<MeshAsset>(emitter_node["MeshUUID"].as<uint64_t>());
			p_res->p_mesh = p_res->p_mesh ? p_res->p_mesh : AssetManager::GetAsset<MeshAsset>(ORNG_BASE_CUBE_ID);

			auto materials = emitter_node["Materials"];
			std::vector<uint64_t> material_ids = materials.as<std::vector<uint64_t>>();
			p_res->materials.resize(p_res->p_mesh->num_materials);

			for (int i = 0; i < p_res->materials.size(); i++) {
				auto* p_mat = AssetManager::GetAsset<Material>(material_ids[i]);
				p_res->materials[i] = p_mat ? p_mat : AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID);
			}
		}

		int dif = p_emitter->m_num_particles - ParticleEmitterComponent::BASE_NUM_PARTICLES;
		p_emitter->DispatchUpdateEvent(ParticleEmitterComponent::FULL_UPDATE, &dif);
	}

	void SceneSerializer::DeserializeTransformComp(const YAML::Node& node, SceneEntity& entity) {
		auto* p_transform = entity.GetComponent<TransformComponent>();
		p_transform->m_pos = node["Pos"].as<glm::vec3>();
		p_transform->m_scale = node["Scale"].as<glm::vec3>();
		p_transform->m_orientation = node["Orientation"].as<glm::vec3>();
		p_transform->m_is_absolute = node["Absolute"].as<bool>();
		p_transform->RebuildMatrix(TransformComponent::UpdateType::ALL);
	}

	void SceneSerializer::DeserializeParticleBufferComp(const YAML::Node& node, SceneEntity& entity) {
		auto* p_buffer = entity.AddComponent<ParticleBufferComponent>();
		p_buffer->m_buffer_id = node["BufferID"].as<uint32_t>();
		p_buffer->m_min_allocated_particles = node["Min allocated particles"].as<uint32_t>();

		Events::ECS_Event<ParticleBufferComponent> e_event{ e_event.event_type = Events::ECS_EventType::COMP_UPDATED, p_buffer };
		Events::EventManager::DispatchEvent(e_event);
	}

	void SceneSerializer::SerializeEntityNodeRef(YAML::Emitter& out, const EntityNodeRef& ref) {
		out << YAML::BeginMap;

		out << "Instructions" << YAML::Flow << YAML::BeginSeq;
		for (const auto& instruction : ref.GetInstructions()) {
			out << instruction;
		}
		out << YAML::EndSeq << YAML::EndMap;
	}

	void SceneSerializer::DeserializeEntityNodeRef(const YAML::Node& node, EntityNodeRef& ref) {
		for (auto instruction : node["Instructions"]) {
			ref.m_instructions.push_back(instruction.as<std::string>());
		}

		//ref.m_target_node_id = node["TargetNodeID"].as<uint32_t>();
	}

	void SceneSerializer::DeserializeMeshComp(const YAML::Node& mesh_node, SceneEntity& entity) {
#ifdef ORNG_ENABLE_TRACY_PROFILE
		ZoneScoped;
#endif
		uint64_t mesh_asset_id = mesh_node["MeshAssetID"].as<uint64_t>();
		auto* p_mesh_asset = AssetManager::GetAsset<MeshAsset>(mesh_asset_id);

		auto& materials = mesh_node["Materials"];
		std::vector<const Material*> material_vec;
		material_vec.resize(materials.size());

		for (int i = 0; i < materials.size(); i++) { 
			auto* p_mat = AssetManager::GetAsset<Material>(materials[i].as<uint64_t>());
			material_vec[i] = p_mat ? p_mat : AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID);
		}

		entity.AddComponent<MeshComponent>(p_mesh_asset ? p_mesh_asset : AssetManager::GetAsset<MeshAsset>(ORNG_BASE_CUBE_ID), std::move(material_vec));
	}


	void SceneSerializer::DeserializeEntity(Scene& scene, YAML::Node& entity_node, SceneEntity& entity, bool ignore_parent) {
#ifdef ORNG_ENABLE_TRACY_PROFILE
		ZoneScoped;
#endif

		uint64_t parent_id = entity_node["ParentID"].as<uint64_t>();
		if (!ignore_parent && parent_id != 0 && entity.GetParent() == entt::null) // Parent may be set externally (prefab deserialization)
			entity.SetParent(*scene.GetEntity(parent_id)); 

		entity.name = entity_node["Name"].as<std::string>();

		std::unordered_map<std::string, std::function<void()>> deserializers = {
			{"TransformComp",[&] { DeserializeTransformComp(entity_node["TransformComp"], entity); }}, 
			{"MeshComp",[&] { DeserializeMeshComp(entity_node["MeshComp"], entity); }},
			{"PointlightComp",[&] { DeserializePointlightComp(entity_node["PointlightComp"], entity); }},
			{"SpotlightComp",[&] { DeserializeSpotlightComp(entity_node["SpotlightComp"], entity); }},
			{"CameraComp",[&] { DeserializeCameraComp(entity_node["CameraComp"], entity); }},
			{"ScriptComp",[&] { DeserializeScriptComp(entity_node["ScriptComp"], entity); }},
			{"AudioComp",[&] { DeserializeAudioComp(entity_node["AudioComp"], entity); }},
			{"ParticleEmitterComp",[&] { DeserializeParticleEmitterComp(entity_node["ParticleEmitterComp"], entity); }},
			{"ParticleBufferComp",[&] { DeserializeParticleBufferComp(entity_node["ParticleBufferComp"], entity); }},
		};

		auto it = entity_node.begin();
		// Skip the non-component fields
		std::advance(it, 3);
		for (it; it != entity_node.end(); it++) {
			auto tag = it->first.as<std::string>();

			if (deserializers.contains(tag)) deserializers[tag]();
		}

		Events::EventManager::DispatchEvent(EntitySerializationEvent{&entity, &entity_node});
		Events::EventManager::DispatchEvent(EntitySerializationEvent{&entity});
	}

	void SceneSerializer::SerializeSceneUUIDs(const Scene& scene, std::string& output) {
		std::unordered_set<std::string> names_taken;

		std::ofstream fout{ output };
		fout << "#pragma once" << "\n";
		fout << "namespace World {\n";
		fout << "namespace Entities {\n";

		for (auto* p_entity : scene.m_entities) {
			std::string ent_name = p_entity->name;
			// Replace spaces with underscores
			std::ranges::for_each(ent_name, [](char& c) {if (c == ' ') c = '_'; });
			// Ensure a unique name
			if (names_taken.contains(ent_name)) {
				int iter = 1;
				while (names_taken.contains(ent_name)) {
					ent_name = ent_name + "_" + std::to_string(iter);
				}
			}
			names_taken.emplace(ent_name);
			fout << "constexpr uint64_t " << ent_name << " = " << p_entity->GetUUID() << ";\n";
		}
		fout << "};"; // namespace Entities

		fout << "namespace Prefabs {\n";
		for (auto* p_prefab : AssetManager::GetView<Prefab>()) {
			std::string prefab_name = p_prefab->filepath.substr(p_prefab->filepath.rfind("\\") + 1);
			prefab_name = prefab_name.substr(0, prefab_name.find(".opfb"));
			std::ranges::for_each(prefab_name, [](char& c) {if (std::isalnum(c) == 0) c = '_'; });
			fout << "constexpr uint64_t " << prefab_name << " = " << p_prefab->uuid() << ";\n";
		}
		fout << "};"; // namespace Prefabs

		fout << "namespace Sounds {\n";
		for (auto* p_asset : AssetManager::GetView<SoundAsset>()) {
			std::string name = p_asset->filepath.substr(p_asset->filepath.rfind("\\") + 1);
			name = name.substr(0, name.rfind("."));
			std::ranges::for_each(name, [](char& c) {if (std::isalnum(c) == 0) c = '_'; });
			fout << "constexpr uint64_t " << name << " = " << p_asset->uuid() << "; \n";
		}
		fout << "};"; // namespace Sounds

		fout << "namespace Materials {\n";
		for (auto* p_asset : AssetManager::GetView<Material>()) {
			std::string name = p_asset->name;
			std::ranges::for_each(name, [](char& c) {if (std::isalnum(c) == 0) c = '_'; });
			fout << "constexpr uint64_t " << name << "_" << p_asset->uuid() << " = " << p_asset->uuid() << "; \n";
		}
		fout << "};"; // namespace Materials

		fout << "};"; // namespace Scene
	}

	void SceneSerializer::SerializeScene(Scene& scene, std::string& output, bool write_to_string) {
		YAML::Emitter out;

		out << YAML::BeginMap;

		out << YAML::Key << "Scene" << YAML::Value << scene.m_name;

		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		for (auto* p_entity : scene.m_entities) {
			SerializeEntity(*p_entity, out);
		}
		out << YAML::EndSeq;

		out << YAML::Key << "DirLight" << YAML::BeginMap;
		out << YAML::Key << "Shadows" << YAML::Value << scene.directional_light.shadows_enabled;
		out << YAML::Key << "Colour" << YAML::Value << scene.directional_light.colour;
		out << YAML::Key << "Direction" << YAML::Value << scene.directional_light.GetLightDirection();
		out << YAML::Key << "CascadeRanges" << YAML::Value << glm::vec3(scene.directional_light.cascade_ranges[0], scene.directional_light.cascade_ranges[1], scene.directional_light.cascade_ranges[2]);
		out << YAML::Key << "Zmults" << YAML::Value << glm::vec3(scene.directional_light.z_mults[0], scene.directional_light.z_mults[1], scene.directional_light.z_mults[2]);
		out << YAML::EndMap;

		out << YAML::Key << "Fog" << YAML::BeginMap;
		Out(out, "Density", scene.post_processing.global_fog.density_coef);
		Out(out, "Absorption", scene.post_processing.global_fog.absorption_coef);
		Out(out, "Scattering", scene.post_processing.global_fog.scattering_coef);
		Out(out, "Anisotropy", scene.post_processing.global_fog.scattering_anisotropy);
		Out(out, "Colour", scene.post_processing.global_fog.colour);
		Out(out, "Steps", scene.post_processing.global_fog.step_count);
		Out(out, "Emission", scene.post_processing.global_fog.emissive_factor);
		out << YAML::EndMap;

		out << YAML::Key << "Bloom" << YAML::BeginMap;
		out << YAML::Key << "Intensity" << YAML::Value << scene.post_processing.bloom.intensity;
		out << YAML::Key << "Knee" << YAML::Value << scene.post_processing.bloom.knee;
		out << YAML::Key << "Threshold" << YAML::Value << scene.post_processing.bloom.threshold;
		out << YAML::EndMap;

		out << YAML::EndMap;

		// Write to either the string or an output file
		if (write_to_string) {
			output = out.c_str();
		}
		else {
			std::ofstream fout{ output };
			fout << out.c_str();
		}

		Events::EventManager::DispatchEvent(SceneSerializationEvent{&out, scene});
	}

	bool SceneSerializer::DeserializeScene(Scene& scene, const std::string& input, bool input_is_filepath) {
		YAML::Node data;

		// Load yaml from either file or string itself
		if (input_is_filepath) {
			std::stringstream str_stream;
			std::ifstream stream(input);
			str_stream << stream.rdbuf();
			data = YAML::Load(str_stream.str());
		}
		else {
			data = YAML::Load(input);
		}

		if (!data.IsDefined() || data.IsNull() || !data["Scene"])
			return false;

		std::string scene_name = data["Scene"].as<std::string>();
		ORNG_CORE_TRACE("Deserializing scene '{0}'", scene_name);

		auto entities = data["Entities"];
		//Create entities in first pass so they can be linked as parent/children in 2nd pass
		for (auto entity_node : entities) {
			scene.CreateEntity(entity_node["Name"].as<std::string>(), entity_node["Entity"].as<uint64_t>());
		}
		for (auto entity_node : entities) {
			DeserializeEntity(scene, entity_node, *scene.GetEntity(entity_node["Entity"].as<uint64_t>()));
		}

		// Resolve/connect any node refs now scene tree is fully built
		for (auto* p_entity : scene.m_entities) {
			Events::EventManager::DispatchEvent(EntitySerializationEvent{p_entity});
		}

		// Directional light
		{
			auto dir_light = data["DirLight"];
			scene.directional_light.colour = dir_light["Colour"].as<glm::vec3>();
			scene.directional_light.shadows_enabled = dir_light["Shadows"].as<bool>();
			scene.directional_light.SetLightDirection(dir_light["Direction"].as<glm::vec3>());
			glm::vec3 cascade_ranges = dir_light["CascadeRanges"].as<glm::vec3>();
			scene.directional_light.cascade_ranges = std::array<float, 3>{cascade_ranges.x, cascade_ranges.y, cascade_ranges.z};
			glm::vec3 zmults = dir_light["Zmults"].as<glm::vec3>();
			scene.directional_light.z_mults = std::array<float, 3>{zmults.x, zmults.y, zmults.z};
		}

		// Bloom
		{
			auto bloom = data["Bloom"];
			scene.post_processing.bloom.intensity = bloom["Intensity"].as<float>();
			scene.post_processing.bloom.threshold = bloom["Threshold"].as<float>();
			scene.post_processing.bloom.knee = bloom["Knee"].as<float>();
		}

		// Fog
		{
			auto fog = data["Fog"];
			scene.post_processing.global_fog.density_coef = fog["Density"].as<float>();
			scene.post_processing.global_fog.absorption_coef = fog["Absorption"].as<float>();
			scene.post_processing.global_fog.scattering_coef = fog["Scattering"].as<float>();
			scene.post_processing.global_fog.scattering_anisotropy = fog["Anisotropy"].as<float>();
			scene.post_processing.global_fog.emissive_factor = fog["Emission"].as<float>();
			scene.post_processing.global_fog.colour = fog["Colour"].as<glm::vec3>();
			scene.post_processing.global_fog.step_count = fog["Steps"].as<int>();
		}

		Events::EventManager::DispatchEvent(SceneSerializationEvent{&data, scene});
		return true;
	}
}