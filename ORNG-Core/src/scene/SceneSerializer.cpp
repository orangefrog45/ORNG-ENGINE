#include "pch/pch.h"

#include <yaml-cpp/yaml.h>




#include "scene/SceneSerializer.h"
#include "scene/Scene.h"
#include "scene/SceneEntity.h"
#include "components/ComponentAPI.h"
#include "rendering/Textures.h"
#include "core/CodedAssets.h"
#include "assets/AssetManager.h"

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


	std::string SceneSerializer::SerializeEntityIntoString(SceneEntity& entity) {
		YAML::Emitter out;

		SerializeEntity(entity, out);
		return std::string{out.c_str()};
	}

	void SceneSerializer::DeserializeEntityFromString(Scene& scene, const std::string& str, SceneEntity& entity) {
		YAML::Node data = YAML::Load(str);
		DeserializeEntity(scene, data, entity);
	}



	void SceneSerializer::SerializeEntity(SceneEntity& entity, YAML::Emitter& out) {
		out << YAML::BeginMap;
		out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();
		out << YAML::Key << "Name" << YAML::Value << entity.name;
		out << YAML::Key << "ParentID" << YAML::Value << (entity.GetParent() ? entity.GetParent()->GetUUID() : 0);

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
			out << YAML::Key << "Direction" << YAML::Value << p_spotlight->m_light_direction_vec;

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

			out << YAML::EndMap;

		}

		ScriptComponent* p_script_comp = entity.GetComponent<ScriptComponent>();
		if (p_script_comp) {
			out << YAML::Key << "ScriptComp" << YAML::BeginMap;
			out << YAML::Key << "ScriptPath" << YAML::Value << p_script_comp->script_filepath;
			out << YAML::EndMap;
		}

		/*CameraComponent* p_cam = entity.GetComponent<CameraComponent>();
		if (p_cam) {
				out << YAML::Key << "CamComp" << YAML::BeginMap;
				out << YAML::Key << "Target"
			out << YAML::EndMap;
		}*/


		out << YAML::EndMap;
	}





	void SceneSerializer::DeserializeEntity(Scene& scene, YAML::Node& entity_node, SceneEntity& entity) {
		uint64_t parent_id = entity_node["ParentID"].as<uint64_t>();
		if (parent_id != 0)
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
				auto* p_mesh_comp = entity.AddComponent<MeshComponent>(mesh_asset_id == ORNG_CUBE_MESH_UUID ? &CodedAssets::GetCubeAsset() : AssetManager::GetAsset<MeshAsset>(mesh_asset_id));

				auto materials = mesh_node["Materials"];
				std::vector<uint64_t> ids = materials.as<std::vector<uint64_t>>();
				for (int i = 0; i < p_mesh_comp->m_materials.size(); i++) { // Material slots automatically allocated for mesh asset through AddComponent<MeshComponent>, keep it within this range
					p_mesh_comp->m_materials[i] = ids[i] == 0 ? AssetManager::GetEmptyMaterial() : AssetManager::GetAsset<Material>(ids[i]);
				}
				scene.m_mesh_component_manager.SortMeshIntoInstanceGroup(p_mesh_comp);

			}


			if (tag == "PhysicsComp") {
				auto physics_node = entity_node["PhysicsComp"];
				auto* p_physics_comp = entity.AddComponent<PhysicsComponent>();

				p_physics_comp->UpdateGeometry(static_cast<PhysicsComponent::GeometryType>(physics_node["GeometryType"].as<unsigned int>()));
				p_physics_comp->SetBodyType(static_cast<PhysicsComponent::RigidBodyType>(physics_node["RigidBodyType"].as<unsigned int>()));
			}


			if (tag == "PointlightComp") {
				auto light_node = entity_node["PointlightComp"];
				auto* p_pointlight_comp = entity.AddComponent<PointLightComponent>();
				p_pointlight_comp->color = light_node["Colour"].as<glm::vec3>();
				p_pointlight_comp->attenuation.constant = light_node["AttenConstant"].as<float>();
				p_pointlight_comp->attenuation.linear = light_node["AttenLinear"].as<float>();
				p_pointlight_comp->attenuation.exp = light_node["AttenExp"].as<float>();
			}

			if (tag == "SpotlightComp") {
				auto light_node = entity_node["SpotlightComp"];
				auto* p_spotlight_comp = entity.AddComponent<SpotLightComponent>();
				p_spotlight_comp->color = light_node["Colour"].as<glm::vec3>();
				p_spotlight_comp->attenuation.constant = light_node["AttenConstant"].as<float>();
				p_spotlight_comp->attenuation.linear = light_node["AttenLinear"].as<float>();
				p_spotlight_comp->attenuation.exp = light_node["AttenExp"].as<float>();
				p_spotlight_comp->m_aperture = light_node["Aperture"].as<float>();
				p_spotlight_comp->m_light_direction_vec = light_node["Direction"].as<glm::vec3>();
			}

			if (tag == "ScriptComp") {
				auto script_node = entity_node["ScriptComp"];
				auto* p_script_comp = entity.AddComponent<ScriptComponent>();
				std::string script_filepath = script_node["ScriptPath"].as<std::string>();
				p_script_comp->script_filepath = script_filepath;

				auto* p_asset = AssetManager::GetAsset<ScriptAsset>(script_filepath);
				if (!p_asset)
					p_asset = AssetManager::AddAsset(new ScriptAsset(script_filepath));

				if (p_asset) {
					p_script_comp->SetSymbols(&p_asset->symbols);
				}
				else {
					ORNG_CORE_ERROR("Scene deserialization error: no script file with filepath '{0}' found", script_filepath);
				}
			}
		}


	}


	void SceneSerializer::SerializeSceneUUIDs(const Scene& scene, std::string& output) {
		std::unordered_set<std::string> names_taken;

		std::ofstream fout{output};
		fout << "#pragma once" << "\n";
		fout << "namespace ScriptInterface {\n";
		fout << "namespace Scene {\n";
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
				std::ranges::for_each(prefab_name, [](char& c) {if (c == ' ') c = '_'; });
				fout << "inline static const std::string " << prefab_name << " = R\"(" << p_prefab->serialized_content << ")\"; \n";
			}
		}
		fout << "};"; // namespace Prefabs
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
			std::ofstream fout{output};
			fout << out.c_str();
		}

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
		auto skybox = data["Skybox"];
		scene.skybox.LoadEnvironmentMap(skybox["HDR filepath"].as<std::string>());

		auto bloom = data["Bloom"];
		scene.post_processing.bloom.intensity = bloom["Intensity"].as<float>();
		scene.post_processing.bloom.threshold = bloom["Threshold"].as<float>();
		scene.post_processing.bloom.knee = bloom["Knee"].as<float>();

	}





}