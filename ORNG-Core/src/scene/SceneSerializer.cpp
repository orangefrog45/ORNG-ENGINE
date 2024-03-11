#include "pch/pch.h"


#include "scene/SceneSerializer.h"
#include "scene/Scene.h"
#include "scene/SceneEntity.h"
#include "assets/AssetManager.h"
#include "util/InterpolatorSerializer.h"
#include "physics/Physics.h"
/*
	struct VertexData {
			std::vector<glm::vec3> positions;
			std::vector<glm::vec3> normals;
			std::vector<glm::vec3> tangents;
			std::vector<glm::vec2> tex_coords;
			std::vector<unsigned int> indices;
		};
*/


namespace YAML {
	template<>
	struct convert<glm::vec3> {
		static Node encode(const glm::vec3& rhs) {
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs) {
			if (!node.IsSequence() || node.size() != 3) {
				return false;
			}

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec4> {
		static Node encode(const glm::vec4& rhs) {
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs) {
			if (!node.IsSequence() || node.size() != 4) {
				return false;
			}

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec2> {
		static Node encode(const glm::vec2& rhs) {
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& rhs) {
			if (!node.IsSequence() || node.size() != 2) {
				return false;
			}

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			return true;
		}
	};
}



namespace ORNG {
	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v) {
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4 v) {
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2 v) {
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

	template<typename T>
	void Out(YAML::Emitter& out, const std::string& key, const T& v) {
		out << YAML::Key << key << YAML::Value << v;
	}

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


	SceneEntity& SceneSerializer::DeserializeEntityUUIDFromString(Scene& scene, const std::string& str) {
		YAML::Node data = YAML::Load(str);
		//Create entities in first pass so they can be linked as parent/children in 2nd pass
		return scene.CreateEntity(data["Name"].as<std::string>(), data["Entity"].as<uint64_t>());
	}

	std::vector<SceneEntity*> SceneSerializer::DeserializePrefabFromString(Scene& scene, const std::string& str) {
		YAML::Node data = YAML::Load(str);

		std::vector<SceneEntity*> ents;

		// key = serialized uuid, val = instantiation uuid
		std::map<uint64_t, uint64_t> id_mappings;

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

		return ents;
	}


	void SceneSerializer::DeserializeEntityFromString(Scene& scene, const std::string& str, SceneEntity& entity) {
		YAML::Node data = YAML::Load(str);
		DeserializeEntity(scene, data, entity);
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

			out << YAML::Key << "Colour" << YAML::Value << p_pointlight->color;
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

			out << YAML::Key << "Colour" << YAML::Value << p_spotlight->color;
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

		PhysicsComponent* p_physics_comp = entity.GetComponent<PhysicsComponent>();
		if (!p_physics_comp) // Check for both types
			p_physics_comp = static_cast<PhysicsComponent*>(entity.GetComponent<PhysicsComponent>());

		if (p_physics_comp) {
			out << YAML::Key << "PhysicsComp";
			out << YAML::BeginMap;

			out << YAML::Key << "RigidBodyType" << YAML::Value << p_physics_comp->m_body_type;
			out << YAML::Key << "GeometryType" << YAML::Value << p_physics_comp->m_geometry_type;
			out << YAML::Key << "IsTrigger" << YAML::Value << p_physics_comp->IsTrigger();
			Out(out, "MaterialUUID", p_physics_comp->p_material->uuid());
			out << YAML::EndMap;
		}

		ScriptComponent* p_script_comp = entity.GetComponent<ScriptComponent>();
		if (p_script_comp) {
			out << YAML::Key << "ScriptComp" << YAML::BeginMap;
			out << YAML::Key << "ScriptPath" << YAML::Value << p_script_comp->script_filepath;
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

		if (auto* p_vehicle = entity.GetComponent<VehicleComponent>()) {
			Out(out, "VehicleComp", YAML::BeginMap);
			Out(out, "BodyMesh", p_vehicle->p_body_mesh->uuid());
			Out(out, "WheelMesh", p_vehicle->p_wheel_mesh->uuid());

			Out(out, "WheelScale", p_vehicle->wheel_scale);
			Out(out, "BodyScale", p_vehicle->body_scale);


			Out(out, "BodyMaterials", YAML::Flow);
			out << YAML::BeginSeq;
			for (auto* p_material : p_vehicle->m_body_materials) {
				out << p_material->uuid();
			}
			out << YAML::EndSeq;

			Out(out, "WheelMaterials", YAML::Flow);
			out << YAML::BeginSeq;
			for (auto* p_material : p_vehicle->m_wheel_materials) {
				out << p_material->uuid();
			}
			out << YAML::EndSeq;

			for (int i = 0; i < 4; i++) {
				Out(out, std::format("Wheel{}", i), YAML::BeginMap);
				Out(out, "SuspensionAttachment", ConvertVec3<PxVec3, glm::vec3>(p_vehicle->m_vehicle.mBaseParams.suspensionParams[i].suspensionAttachment.p));
				out << YAML::EndMap;
			}

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

		if (auto* p_controller = entity.GetComponent<CharacterControllerComponent>()) {
			auto* p_capsule = static_cast<PxCapsuleController*>(p_controller->p_controller);
			Out(out, "CharacterControllerComp", YAML::BeginMap);
			Out(out, "Height", p_capsule->getHeight());
			Out(out, "Radius", p_capsule->getRadius());

			out << YAML::EndMap;
		}

		out << YAML::EndMap;
	}





	void SceneSerializer::DeserializeEntity(Scene& scene, YAML::Node& entity_node, SceneEntity& entity) {
		auto* p_replacement_material = AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID);

		uint64_t parent_id = entity_node["ParentID"].as<uint64_t>();
		if (parent_id != 0 && entity.GetParent() == entt::null) // Parent may be set externally (prefab deserialization)
			entity.SetParent(*scene.GetEntity(parent_id));

		auto transform_node = entity_node["TransformComp"];
		auto* p_transform = entity.GetComponent<TransformComponent>();
		p_transform->SetPosition(transform_node["Pos"].as<glm::vec3>());
		p_transform->SetScale(transform_node["Scale"].as<glm::vec3>());
		p_transform->SetOrientation(transform_node["Orientation"].as<glm::vec3>());
		p_transform->SetAbsoluteMode(transform_node["Absolute"].as<bool>());

		entity.name = entity_node["Name"].as<std::string>();

		for (auto node : entity_node) {
			if (!node.first.IsDefined())
				continue;


			std::string tag = node.first.as<std::string>();
			if (tag == "MeshComp") {
				auto mesh_node = entity_node["MeshComp"];
				uint64_t mesh_asset_id = mesh_node["MeshAssetID"].as<uint64_t>();
				auto* p_mesh_asset = AssetManager::GetAsset<MeshAsset>(mesh_asset_id);
				auto* p_mesh_comp = entity.AddComponent<MeshComponent>(p_mesh_asset ? p_mesh_asset : AssetManager::GetAsset<MeshAsset>(ORNG_BASE_MESH_ID));

				auto materials = mesh_node["Materials"];
				std::vector<uint64_t> ids = materials.as<std::vector<uint64_t>>();
				p_mesh_comp->m_materials.resize(ids.size());

;				for (int i = 0; i < ids.size(); i++) { // Material slots automatical;ly allocated for mesh asset through AddComponent<MeshComponent>, keep it within this range
					auto* p_mat = AssetManager::GetAsset<Material>(ids[i]);
					p_mesh_comp->m_materials[i] = p_mat ? p_mat : p_replacement_material;
				}

				scene.m_mesh_component_manager.SortMeshIntoInstanceGroup(p_mesh_comp);
			}


			if (tag == "PhysicsComp") {
				auto physics_node = entity_node["PhysicsComp"];
				auto* p_physics_comp = entity.AddComponent<PhysicsComponent>();

				p_physics_comp->UpdateGeometry(static_cast<PhysicsComponent::GeometryType>(physics_node["GeometryType"].as<unsigned int>()));
				p_physics_comp->SetBodyType(static_cast<PhysicsComponent::RigidBodyType>(physics_node["RigidBodyType"].as<unsigned int>()));
				p_physics_comp->SetTrigger(physics_node["IsTrigger"].as<bool>());

				auto* p_material = AssetManager::GetAsset<PhysXMaterialAsset>(physics_node["MaterialUUID"].as<uint64_t>());
				p_physics_comp->SetMaterial(p_material ? *p_material : *AssetManager::GetAsset<PhysXMaterialAsset>(ORNG_BASE_PHYSX_MATERIAL_ID));
			}


			if (tag == "PointlightComp") {
				auto light_node = entity_node["PointlightComp"];
				auto* p_pointlight_comp = entity.AddComponent<PointLightComponent>();
				p_pointlight_comp->color = light_node["Colour"].as<glm::vec3>();
				p_pointlight_comp->attenuation.constant = light_node["AttenConstant"].as<float>();
				p_pointlight_comp->attenuation.linear = light_node["AttenLinear"].as<float>();
				p_pointlight_comp->attenuation.exp = light_node["AttenExp"].as<float>();
				p_pointlight_comp->shadows_enabled = light_node["Shadows"].as<bool>();
				p_pointlight_comp->shadow_distance = light_node["ShadowDistance"].as<float>();
			}

			if (tag == "SpotlightComp") {
				auto light_node = entity_node["SpotlightComp"];
				auto* p_spotlight_comp = entity.AddComponent<SpotLightComponent>();
				p_spotlight_comp->color = light_node["Colour"].as<glm::vec3>();
				p_spotlight_comp->attenuation.constant = light_node["AttenConstant"].as<float>();
				p_spotlight_comp->attenuation.linear = light_node["AttenLinear"].as<float>();
				p_spotlight_comp->attenuation.exp = light_node["AttenExp"].as<float>();
				p_spotlight_comp->m_aperture = light_node["Aperture"].as<float>();
				p_spotlight_comp->shadows_enabled = light_node["Shadows"].as<bool>();
				p_spotlight_comp->shadow_distance = light_node["ShadowDistance"].as<float>();
			}

			if (tag == "CameraComp") {
				auto cam_node = entity_node["CameraComp"];
				auto* p_cam_comp = entity.AddComponent<CameraComponent>();
				p_cam_comp->fov = cam_node["FOV"].as<float>();
				p_cam_comp->exposure = cam_node["Exposure"].as<float>();
				p_cam_comp->zFar = cam_node["zFar"].as<float>();
				p_cam_comp->zNear = cam_node["zNear"].as<float>();
			}

			if (tag == "ScriptComp") {
				auto script_node = entity_node["ScriptComp"];
				auto* p_script_comp = entity.AddComponent<ScriptComponent>();
				std::string script_filepath = script_node["ScriptPath"].as<std::string>();
				p_script_comp->script_filepath = script_filepath;

				auto* p_asset = AssetManager::GetAsset<ScriptAsset>(script_filepath);
				if (!p_asset)
					p_asset = AssetManager::GetAsset< ScriptAsset>(ORNG_BASE_SCRIPT_ID);

				if (p_asset) {
					p_script_comp->SetSymbols(&p_asset->symbols);
				}
				else {
					ORNG_CORE_ERROR("Scene deserialization error: no script file with filepath '{0}' found", script_filepath);
				}
			}

			if (tag == "AudioComp") {
				auto audio_node = entity_node["AudioComp"];
				auto* p_comp = entity.AddComponent<AudioComponent>();
				p_comp->SetMinMaxRange(audio_node["MinRange"].as<float>(), audio_node["MaxRange"].as<float>());
				p_comp->SetPitch(audio_node["Pitch"].as<float>());
				p_comp->SetVolume(audio_node["Volume"].as<float>());
				p_comp->SetSoundAssetUUID(audio_node["AudioUUID"].as<uint64_t>());
			}


			if (tag == "VehicleComp") {
				auto vehicle_node = entity_node["VehicleComp"];
				auto* p_comp = entity.AddComponent<VehicleComponent>();
				p_comp->p_body_mesh = AssetManager::GetAsset<MeshAsset>(vehicle_node["BodyMesh"].as<uint64_t>());
				p_comp->p_body_mesh = p_comp->p_body_mesh ? p_comp->p_body_mesh : AssetManager::GetAsset<MeshAsset>(ORNG_BASE_MESH_ID);

				p_comp->p_wheel_mesh = AssetManager::GetAsset<MeshAsset>(vehicle_node["WheelMesh"].as<uint64_t>());
				p_comp->p_wheel_mesh = p_comp->p_wheel_mesh ? p_comp->p_wheel_mesh : AssetManager::GetAsset<MeshAsset>(ORNG_BASE_MESH_ID);


				p_comp->body_scale = vehicle_node["BodyScale"].as<glm::vec3>();
				p_comp->wheel_scale = vehicle_node["WheelScale"].as<glm::vec3>();

				auto body_materials = vehicle_node["BodyMaterials"];
				std::vector<uint64_t> body_ids = body_materials.as<std::vector<uint64_t>>();
				p_comp->m_body_materials.resize(p_comp->p_body_mesh->num_materials);
				for (int i = 0; i < p_comp->p_body_mesh->num_materials; i++) {
					auto* p_mat = AssetManager::GetAsset<Material>(body_ids[i]);
					p_comp->m_body_materials[i] = p_mat ? p_mat : p_replacement_material;
				}

				auto wheel_materials = vehicle_node["WheelMaterials"];
				std::vector<uint64_t> wheel_ids = wheel_materials.as<std::vector<uint64_t>>();
				p_comp->m_wheel_materials.resize(p_comp->p_wheel_mesh->num_materials);
				for (int i = 0; i < p_comp->p_wheel_mesh->num_materials; i++) {
					auto* p_mat = AssetManager::GetAsset<Material>(wheel_ids[i]);
					p_comp->m_wheel_materials[i] = p_mat ? p_mat : p_replacement_material;
				}
				for (int i = 0; i < 4; i++) {
					auto wheel = vehicle_node[std::format("Wheel{}", i)];
					p_comp->m_vehicle.mBaseParams.suspensionParams[i].suspensionAttachment.p = ConvertVec3<glm::vec3, PxVec3>(wheel["SuspensionAttachment"].as<glm::vec3>());
				}

				scene.physics_system.InitVehicle(p_comp);
			}


			
			if (tag == "ParticleEmitterComp") {
				auto emitter_node = entity_node["ParticleEmitterComp"];
				auto* p_emitter = entity.AddComponent<ParticleEmitterComponent>();
				p_emitter->m_spread = emitter_node["Spread"].as<float>();
				p_emitter->m_spawn_extents = emitter_node["Spawn extents"].as<glm::vec3>();
				p_emitter->m_velocity_min_max_scalar = emitter_node["Velocity range"].as<glm::vec2>();
				p_emitter->m_num_particles = emitter_node["Nb. particles"].as<unsigned>();
				p_emitter->m_particle_lifespan_ms = emitter_node["Lifespan"].as<float>();
				p_emitter->m_particle_spawn_delay_ms = emitter_node["Spawn delay"].as<float>();
				p_emitter->m_type = static_cast<ParticleEmitterComponent::EmitterType>(emitter_node["Type"].as<unsigned>());
				p_emitter->acceleration = emitter_node["Acceleration"].as<glm::vec3>();

				InterpolatorSerializer::DeserializeInterpolator(emitter_node["Alpha over time"], p_emitter->m_life_alpha_interpolator);
				InterpolatorSerializer::DeserializeInterpolator(emitter_node["Colour over time"], p_emitter->m_life_colour_interpolator);
				InterpolatorSerializer::DeserializeInterpolator(emitter_node["Scale over time"], p_emitter->m_life_scale_interpolator);
				if (p_emitter->GetType() == ParticleEmitterComponent::BILLBOARD) {
					auto* p_mat = AssetManager::GetAsset<Material>(emitter_node["MaterialUUID"].as<uint64_t>());
					entity.AddComponent<ParticleBillboardResources>()->p_material = p_mat ? p_mat : p_replacement_material;
				}
				else {
					auto* p_res = entity.AddComponent<ParticleMeshResources>();
					p_res->p_mesh = AssetManager::GetAsset<MeshAsset>(emitter_node["MeshUUID"].as<uint64_t>());
					p_res->p_mesh = p_res->p_mesh ? p_res->p_mesh : AssetManager::GetAsset<MeshAsset>(ORNG_BASE_MESH_ID);

					auto materials = emitter_node["Materials"];
					std::vector<uint64_t> material_ids = materials.as<std::vector<uint64_t>>();
					p_res->materials.resize(p_res->p_mesh->num_materials);

					for (int i = 0; i < p_res->materials.size(); i++) {
						auto* p_mat = AssetManager::GetAsset<Material>(material_ids[i]);
						p_res->materials[i] = p_mat ? p_mat : p_replacement_material;
					}
				}

				p_emitter->DispatchUpdateEvent(ParticleEmitterComponent::FULL_UPDATE,(int)p_emitter->m_num_particles - ParticleEmitterComponent::BASE_NUM_PARTICLES);
			}

			if (tag == "ParticleBufferComp") {
				auto buffer_node = entity_node["ParticleBufferComp"];
				auto* p_buffer = entity.AddComponent<ParticleBufferComponent>();
				p_buffer->m_buffer_id = buffer_node["BufferID"].as<uint32_t>();
				p_buffer->m_min_allocated_particles = buffer_node["Min allocated particles"].as<uint32_t>();
				
				Events::ECS_Event<ParticleBufferComponent> e_event{ e_event.event_type = Events::ECS_EventType::COMP_UPDATED, p_buffer };
				Events::EventManager::DispatchEvent(e_event);
			}

			if (tag == "CharacterControllerComp") {
				auto* p_controller = entity.AddComponent<CharacterControllerComponent>();
				PxCapsuleController* p_capsule = static_cast<PxCapsuleController*>(p_controller->p_controller);
				auto c_node = entity_node["CharacterControllerComp"];
				p_capsule->setRadius(c_node["Radius"].as<float>());
				p_capsule->setHeight(c_node["Height"].as<float>());
			}
		}
	}


	void SceneSerializer::SerializeSceneUUIDs(const Scene& scene, std::string& output) {
		std::unordered_set<std::string> names_taken;

		std::ofstream fout{ output };
		fout << "#pragma once" << "\n";
		fout << "namespace ScriptInterface {\n";
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
		for (auto [uuid, p_asset] : AssetManager::Get().m_assets) {
			if (auto* p_prefab = dynamic_cast<Prefab*>(p_asset)) {
				std::string prefab_name = p_prefab->filepath.substr(p_prefab->filepath.rfind("\\") + 1);
				prefab_name = prefab_name.substr(0, prefab_name.find(".opfb"));
				std::ranges::for_each(prefab_name, [](char& c) {if (std::isalnum(c) == 0) c = '_'; });
				fout << "inline static const std::string " << prefab_name << " = R\"(" << p_prefab->serialized_content << ")\"; \n";
			}
		}
		fout << "};"; // namespace Prefabs

		fout << "namespace Sounds {\n";
		for (auto* p_asset : AssetManager::GetView<SoundAsset>()) {
			std::string name = p_asset->filepath.substr(p_asset->filepath.rfind("\\") + 1);
			name = name.substr(0, name.rfind("."));
			std::ranges::for_each(name, [](char& c) {if (std::isalnum(c) == 0) c = '_'; });
			fout << "inline static const uint64_t " << name << " = " << p_asset->uuid() << "; \n";
		}
		fout << "};"; // namespace Sounds

		fout << "};"; // namespace Scene
		fout << "};"; // namespace ScriptInterface
	}

	void SceneSerializer::SerializeScene(const Scene& scene, std::string& output, bool write_to_string) {
		YAML::Emitter out;

		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << scene.m_name;


		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		for (auto* p_entity : scene.m_entities) {
			SerializeEntity(*p_entity, out);
		}

		out << YAML::EndSeq;

		out << YAML::Key << "DirLight" << YAML::BeginMap;
		out << YAML::Key << "Colour" << YAML::Value << scene.directional_light.color;
		out << YAML::Key << "Direction" << YAML::Value << scene.directional_light.GetLightDirection();
		out << YAML::Key << "CascadeRanges" << YAML::Value << glm::vec3(scene.directional_light.cascade_ranges[0], scene.directional_light.cascade_ranges[1], scene.directional_light.cascade_ranges[2]);
		out << YAML::Key << "Zmults" << YAML::Value << glm::vec3(scene.directional_light.z_mults[0], scene.directional_light.z_mults[1], scene.directional_light.z_mults[2]);
		out << YAML::EndMap;


		out << YAML::Key << "Skybox" << YAML::BeginMap;
		out << YAML::Key << "HDR filepath" << YAML::Value << scene.skybox.m_hdr_tex_filepath;
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
	}



	bool SceneSerializer::DeserializeScene(Scene& scene, const std::string& input, bool load_env_map, bool input_is_filepath) {
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


		// Entities
		auto entities = data["Entities"];
		//Create entities in first pass so they can be linked as parent/children in 2nd pass
		for (auto entity_node : entities) {
			scene.CreateEntity(entity_node["Name"].as<std::string>(), entity_node["Entity"].as<uint64_t>());
		}
		for (auto entity_node : entities) {
			DeserializeEntity(scene, entity_node, *scene.GetEntity(entity_node["Entity"].as<uint64_t>()));
		}

		// Directional light
		auto dir_light = data["DirLight"];
		scene.directional_light.color = dir_light["Colour"].as<glm::vec3>();
		scene.directional_light.SetLightDirection(dir_light["Direction"].as<glm::vec3>());
		glm::vec3 cascade_ranges = dir_light["CascadeRanges"].as<glm::vec3>();
		scene.directional_light.cascade_ranges = std::array<float, 3>{cascade_ranges.x, cascade_ranges.y, cascade_ranges.z};
		glm::vec3 zmults = dir_light["Zmults"].as<glm::vec3>();
		scene.directional_light.z_mults = std::array<float, 3>{zmults.x, zmults.y, zmults.z};

		// Skybox/Env map
		if (load_env_map) {
			auto skybox = data["Skybox"];
			scene.skybox.LoadEnvironmentMap(skybox["HDR filepath"].as<std::string>());
		}

		auto bloom = data["Bloom"];
		scene.post_processing.bloom.intensity = bloom["Intensity"].as<float>();
		scene.post_processing.bloom.threshold = bloom["Threshold"].as<float>();
		scene.post_processing.bloom.knee = bloom["Knee"].as<float>();

		return true;
	}
}