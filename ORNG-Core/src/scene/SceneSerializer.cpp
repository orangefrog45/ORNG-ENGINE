#include "pch/pch.h"

#include <yaml-cpp/yaml.h>
#include <bitsery/bitsery.h>
#include <bitsery/traits/vector.h>
#include <bitsery/adapter/stream.h>
#include "bitsery/traits/string.h"



#include "scene/SceneSerializer.h"
#include "scene/Scene.h"
#include "scene/SceneEntity.h"
#include "components/ComponentAPI.h"
#include "rendering/Textures.h"
#include "core/CodedAssets.h"
#include "core/AssetManager.h"

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

namespace bitsery {
	using namespace ORNG;
	template <typename S>
	void serialize(S& s, glm::vec3& o) {
		s.value4b(o.x);
		s.value4b(o.y);
		s.value4b(o.z);
	}

	template <typename S>
	void serialize(S& s, VertexData3D& o) {
		s.container4b(o.positions, ORNG_MAX_MESH_INDICES);
		s.container4b(o.normals, ORNG_MAX_MESH_INDICES);
		s.container4b(o.tangents, ORNG_MAX_MESH_INDICES);
		s.container4b(o.tex_coords, ORNG_MAX_MESH_INDICES);
		s.container4b(o.indices, ORNG_MAX_MESH_INDICES);
	}


	template <typename S>
	void serialize(S& s, AABB& o) {
		s.object(o.max);
		s.object(o.min);
		s.object(o.center);
	}

	template <typename S>
	void serialize(S& s, MeshAsset::MeshEntry& o) {
		s.value4b(o.base_index);
		s.value4b(o.base_vertex);
		s.value4b(o.material_index);
		s.value4b(o.num_indices);
	}
	template<typename S>
	void serialize(S& s, VAO& o) {
		s.object(o.vertex_data);
	}
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

		const auto* p_physics_comp = entity.GetComponent<PhysicsComponent>();

		if (p_physics_comp) {
			out << YAML::Key << "PhysicsComp";
			out << YAML::BeginMap;

			out << YAML::Key << "RigidBodyType" << YAML::Value << p_physics_comp->rigid_body_type;
			out << YAML::Key << "GeometryType" << YAML::Value << p_physics_comp->geometry_type;

			out << YAML::EndMap;

		}

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


		for (auto node : entity_node) {
			if (!node.first.IsDefined())
				continue;


			std::string tag = node.first.as<std::string>();
			if (tag == "MeshComp") {
				auto mesh_node = entity_node["MeshComp"];
				uint64_t mesh_asset_id = mesh_node["MeshAssetID"].as<uint64_t>();
				auto* p_mesh_comp = entity.AddComponent<MeshComponent>(mesh_asset_id == ORNG_CUBE_MESH_UUID ? &CodedAssets::GetCubeAsset() : AssetManager::GetMeshAsset(mesh_asset_id));

				auto materials = mesh_node["Materials"];
				std::vector<uint64_t> ids = materials.as<std::vector<uint64_t>>();
				for (int i = 0; i < p_mesh_comp->m_materials.size(); i++) { // Material slots automatically allocated for mesh asset through AddComponent<MeshComponent>, keep it within this range
					p_mesh_comp->m_materials[i] = AssetManager::GetMaterial(ids[i]);
				}
				scene.m_mesh_component_manager.SortMeshIntoInstanceGroup(p_mesh_comp);

			}


			if (tag == "PhysicsComp") {
				auto physics_node = entity_node["PhysicsComp"];
				auto* p_physics_comp = entity.AddComponent<PhysicsComponent>();
				p_physics_comp->SetBodyType(static_cast<PhysicsComponent::RigidBodyType>(physics_node["RigidBodyType"].as<unsigned int>()));
				p_physics_comp->UpdateGeometry(static_cast<PhysicsComponent::GeometryType>(physics_node["GeometryType"].as<unsigned int>()));
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
		}


	}




	void SceneSerializer::SerializeScene(const Scene& scene, const std::string& filepath) {
		YAML::Emitter out;

		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << scene.m_name;
		out << YAML::Key << "MeshAssets" << YAML::Value << YAML::BeginSeq; // Mesh assets

		for (const auto* p_mesh_asset : AssetManager::Get().m_meshes) {
			out << YAML::BeginMap;
			out << YAML::Key << "MeshAsset" << p_mesh_asset->uuid();
			out << YAML::Key << "Filepath" << YAML::Value << p_mesh_asset->GetFilename();

			out << YAML::Key << "Materials" << YAML::Value;
			out << YAML::Flow;
			out << YAML::BeginSeq;
			for (auto* p_material : p_mesh_asset->m_material_assets) {
				out << p_material->uuid();
			}
			out << YAML::EndSeq;


			out << YAML::EndMap;
		}

		out << YAML::EndSeq; // Mesh assets

		out << YAML::Key << "TextureAssets" << YAML::Value << YAML::BeginSeq; // Texture assets

		for (const auto* p_tex_asset : AssetManager::Get().m_2d_textures) {
			out << YAML::BeginMap;
			out << YAML::Key << "TextureAsset" << p_tex_asset->uuid();
			out << YAML::Key << "Filepath" << YAML::Value << p_tex_asset->GetSpec().filepath;
			out << YAML::Key << "Wrap mode" << YAML::Value << p_tex_asset->GetSpec().wrap_params;
			out << YAML::Key << "Min filter" << YAML::Value << p_tex_asset->GetSpec().min_filter;
			out << YAML::Key << "Mag filter" << YAML::Value << p_tex_asset->GetSpec().mag_filter;
			out << YAML::Key << "SRGB" << YAML::Value << static_cast<uint32_t>(p_tex_asset->GetSpec().srgb_space);
			out << YAML::EndMap;
		}


		out << YAML::EndSeq; // Texture assets

		out << YAML::Key << "Materials" << YAML::Value << YAML::BeginSeq; // Materials

		for (const auto* p_material : AssetManager::Get().m_materials) {

			if (p_material->uuid() == ORNG_REPLACEMENT_MATERIAL_ID) // Always created on startup by scene, doesn't need saving
				continue;

			out << YAML::BeginMap;
			out << YAML::Key << "SceneMaterial" << p_material->uuid();
			out << YAML::Key << "Name" << p_material->name;
			out << YAML::Key << "Base colour texture" << YAML::Value << (p_material->base_color_texture ? p_material->base_color_texture->uuid() : 0);
			out << YAML::Key << "Normal texture" << YAML::Value << (p_material->normal_map_texture ? p_material->normal_map_texture->uuid() : 0);
			out << YAML::Key << "AO texture" << YAML::Value << (p_material->ao_texture ? p_material->ao_texture->uuid() : 0);
			out << YAML::Key << "Metallic texture" << YAML::Value << (p_material->metallic_texture ? p_material->metallic_texture->uuid() : 0);
			out << YAML::Key << "Roughness texture" << YAML::Value << (p_material->roughness_texture ? p_material->roughness_texture->uuid() : 0);
			out << YAML::Key << "Emissive texture" << YAML::Value << (p_material->emissive_texture ? p_material->emissive_texture->uuid() : 0);
			out << YAML::Key << "Base colour" << YAML::Value << p_material->base_color;
			out << YAML::Key << "Metallic" << YAML::Value << p_material->metallic;
			out << YAML::Key << "Roughness" << YAML::Value << p_material->roughness;
			out << YAML::Key << "AO" << YAML::Value << p_material->ao;
			out << YAML::Key << "TileScale" << YAML::Value << p_material->tile_scale;
			out << YAML::Key << "Emissive" << YAML::Value << p_material->emissive;
			out << YAML::Key << "Emissive strength" << YAML::Value << p_material->emissive_strength;
			out << YAML::EndMap;
		}

		out << YAML::EndSeq;


		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

		for (auto* p_entity : scene.m_entities) {
			SerializeEntity(*p_entity, out);
		}

		out << YAML::EndSeq;

		out << YAML::Key << "DirLight" << YAML::BeginMap;
		out << YAML::Key << "Colour" << YAML::Value << scene.m_directional_light.color;
		out << YAML::Key << "Direction" << YAML::Value << scene.m_directional_light.GetLightDirection();
		out << YAML::Key << "CascadeRanges" << YAML::Value << glm::vec3(scene.m_directional_light.cascade_ranges[0], scene.m_directional_light.cascade_ranges[1], scene.m_directional_light.cascade_ranges[2]);
		out << YAML::Key << "Zmults" << YAML::Value << glm::vec3(scene.m_directional_light.z_mults[0], scene.m_directional_light.z_mults[1], scene.m_directional_light.z_mults[2]);
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
		std::ofstream fout{filepath};
		fout << out.c_str();
	}

	bool SceneSerializer::DeserializeScene(Scene& scene, const std::string& filepath) {
		std::ifstream stream(filepath);
		std::stringstream str_stream;
		str_stream << stream.rdbuf();

		YAML::Node data = YAML::Load(str_stream.str());

		if (!data.IsDefined() || data.IsNull() || !data["Scene"])
			return false;

		std::string scene_name = data["Scene"].as<std::string>();
		ORNG_CORE_TRACE("Deserializing scene '{0}'", scene_name);


		// Creating textures
		auto textures = data["TextureAssets"];

		if (textures) {
			Texture2DSpec base_spec;
			for (auto texture : textures) {
				uint64_t id = texture["TextureAsset"].as<uint64_t>();
				std::string tex_filepath = texture["Filepath"].as<std::string>();
				unsigned int wrap_mode = texture["Wrap mode"].as<unsigned int>();
				unsigned int min_filter = texture["Min filter"].as<unsigned int>();
				unsigned int mag_filter = texture["Mag filter"].as<unsigned int>();
				bool srgb = static_cast<bool>(texture["SRGB"].as<uint32_t>());

				base_spec.generate_mipmaps = true;
				base_spec.mag_filter = mag_filter;
				base_spec.min_filter = min_filter;
				base_spec.filepath = tex_filepath;
				base_spec.wrap_params = wrap_mode;
				base_spec.srgb_space = srgb;

				AssetManager::CreateTexture2D(base_spec, id);
			}
		}

		// Creating materials
		auto materials = data["Materials"];

		for (auto material : materials) {
			ORNG_CORE_CRITICAL("SCENE {0}", material["SceneMaterial"].as<uint64_t>());
			auto* p_material = AssetManager::CreateMaterial(material["SceneMaterial"].as<uint64_t>());
			p_material->name = material["Name"].as<std::string>();
			p_material->base_color_texture = AssetManager::GetTexture(material["Base colour texture"].as<uint64_t>());
			p_material->normal_map_texture = AssetManager::GetTexture(material["Normal texture"].as<uint64_t>());
			p_material->ao_texture = AssetManager::GetTexture(material["AO texture"].as<uint64_t>());
			p_material->metallic_texture = AssetManager::GetTexture(material["Metallic texture"].as<uint64_t>());
			p_material->roughness_texture = AssetManager::GetTexture(material["Roughness texture"].as<uint64_t>());
			p_material->emissive_texture = AssetManager::GetTexture(material["Emissive texture"].as<uint64_t>());
			p_material->base_color = material["Base colour"].as<glm::vec3>();
			p_material->metallic = material["Metallic"].as<float>();
			p_material->roughness = material["Roughness"].as<float>();
			p_material->ao = material["AO"].as<float>();
			p_material->tile_scale = material["TileScale"].as<glm::vec2>();
			p_material->emissive = material["Emissive"].as<bool>();
			p_material->emissive_strength = material["Emissive strength"].as<float>();

		}

		// Create mesh assets and link materials
		std::vector<std::vector<Material*>> material_vec; // Used to link assets with their materials below
		auto mesh_assets = data["MeshAssets"];

		if (mesh_assets) {
			for (auto asset : mesh_assets) {
				std::string asset_filepath = asset["Filepath"].as<std::string>();
				uint64_t id = asset["MeshAsset"].as<uint64_t>();


				auto material_ids = asset["Materials"];
				std::vector<Material*> mesh_material_vec;
				for (auto material_id : material_ids) {
					ORNG_CORE_CRITICAL(material_id.as<uint64_t>());
					mesh_material_vec.push_back(AssetManager::GetMaterial(material_id.as<uint64_t>()));
				}
				material_vec.push_back(mesh_material_vec);


				if (std::string serialized_filepath = "./res/meshes/" + asset_filepath.substr(asset_filepath.find_last_of("/") + 1) + ".bin"; std::filesystem::exists(serialized_filepath)) {
					// Load from binary file
					auto* p_asset = AssetManager::CreateMeshAsset(serialized_filepath, id);
					DeserializeMeshAssetBinary(serialized_filepath, *p_asset);
					AssetManager::LoadMeshAssetIntoGL(p_asset, mesh_material_vec);
					AssetManager::DispatchAssetEvent(Events::ProjectEventType::MESH_LOADED, reinterpret_cast<uint8_t*>(p_asset));
				}
				else {
					// Load from source asset file
					auto* p_asset = AssetManager::CreateMeshAsset(asset_filepath, id);
					AssetManager::LoadMeshAssetPreExistingMaterials(p_asset, mesh_material_vec);
				}

			}
		}
		AssetManager::StallUntilMeshesLoaded();

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
		scene.m_directional_light.color = dir_light["Colour"].as<glm::vec3>();
		scene.m_directional_light.SetLightDirection(dir_light["Direction"].as<glm::vec3>());
		glm::vec3 cascade_ranges = dir_light["CascadeRanges"].as<glm::vec3>();
		scene.m_directional_light.cascade_ranges = std::array<float, 3>{cascade_ranges.x, cascade_ranges.y, cascade_ranges.z};
		glm::vec3 zmults = dir_light["Zmults"].as<glm::vec3>();
		scene.m_directional_light.z_mults = std::array<float, 3>{zmults.x, zmults.y, zmults.z};

		// Skybox/Env map
		auto skybox = data["Skybox"];
		scene.skybox.LoadEnvironmentMap(skybox["HDR filepath"].as<std::string>());

		auto bloom = data["Bloom"];
		scene.post_processing.bloom.intensity = bloom["Intensity"].as<float>();
		scene.post_processing.bloom.threshold = bloom["Threshold"].as<float>();
		scene.post_processing.bloom.knee = bloom["Knee"].as<float>();

	}




	void SceneSerializer::SerializeMeshAssetBinary(const std::string& filepath, MeshAsset& data) {
		std::ofstream s{ filepath, s.binary | s.trunc | s.out };
		if (!s.is_open()) {
			ORNG_CORE_ERROR("Vertex serialization error: Cannot open {0} for writing", filepath);
			return;
		}
		// we cannot use quick serialization function, because streams cannot use
// writtenBytesCount method
		bitsery::Serializer<bitsery::OutputBufferedStreamAdapter> ser{ s };
		ser.object(data);
		// flush to writer
		ser.adapter().flush();
		s.close();
	}

	void SceneSerializer::DeserializeMeshAssetBinary(const std::string& filepath, MeshAsset& data) {
		std::ifstream s{ filepath, std::ios::binary };
		if (!s.is_open()) {
			ORNG_CORE_ERROR("Deserialization error: Cannot open {0} for reading", filepath);
			return;
		}

		// Use buffered stream adapter
		bitsery::Deserializer<bitsery::InputStreamAdapter> des{ s };

		// Deserialize individual objects
		des.object(data.m_vao);
		des.object(data.m_aabb);
		//des.object(data.m_aabb);
		uint32_t size;
		des.value4b(size);
		data.m_submeshes.resize(size);
		for (int i = 0; i < size; i++) {
			des.object(data.m_submeshes[i]);
		}

	}
}