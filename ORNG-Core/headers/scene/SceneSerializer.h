#pragma once
#include "util/UUID.h"
#include <bitsery/bitsery.h>
#include <bitsery/traits/vector.h>
#include <bitsery/adapter/stream.h>
#include "bitsery/traits/string.h"
#include "util/Log.h"
#include "EntityNodeRef.h"

namespace YAML {
	class Emitter;
	class Node;
}


namespace ORNG {
	class Scene;
	class SceneEntity;
	class VAO;
	class Texture2D;
	struct VertexData3D;
	struct TextureFileData;
	class MeshAsset;
	class VehicleComponent;
	class MeshComponent;
	struct JointComponent;
	struct Prefab;

	class SceneSerializer {
	public:
		// Output is either the filepath to write to or a string to be written to, if write_to_string is true then the string will be written to, no files
		static void SerializeScene(const Scene& scene, std::string& output, bool write_to_string = false);

		// Produces a .h file with UUID values for each named entity and asset, used in scripts
		static void SerializeSceneUUIDs(const Scene& scene, std::string& output);

		// Deserializes from filepath at "input" if input_is_filepath = true, else deserializes from the string itself assuming it contains valid yaml data
		static bool DeserializeScene(Scene& scene, const std::string& input, bool input_is_filepath = true);

		static void SerializeEntity(SceneEntity& entity, YAML::Emitter& out);

		// Entity argument is the entity that the data will be loaded into
		static void DeserializeEntity(Scene& scene, YAML::Node& entity_node, SceneEntity& entity, bool ignore_parent = false);

		// Called on each entity after the scene tree is fully built, resolves any EntityNodeRefs into actual entities and connects them appropiately (e.g joint connections)
		// Any references to neighbour entities (same parent) that are before "neighbour_start" aren't resolved unless a duplicate is found after it, this allows entity duplication to work nicely with EntityNodeRefs
		static void ResolveEntityRefs(Scene& scene, SceneEntity& entity);

		static std::string SerializeEntityIntoString(SceneEntity& entity);

		static std::string SerializeEntityArrayIntoString(const std::vector<SceneEntity*>& entities);

		// Entity argument is the entity that the data will be loaded into
		static void DeserializeEntityFromString(Scene& scene, const std::string& str, SceneEntity& entity, bool ignore_parent = false);

		// Entities created by this function
		static std::vector<SceneEntity*> DeserializePrefab(Scene& scene, const Prefab& prefab);

		// Any entities which have components that reference other entities via UUID (e.g joints) have these references switched to point to an entity in the lookup map if a valid UUID mapping exists for it
		// Any references that point to entities not contained in the lookup map will not be affected
		// UUID lookup should be {k: old_uuid, v: new_uuid}
		static void RemapEntityReferences(const std::unordered_map<uint64_t, uint64_t>& uuid_lookup, const std::vector<SceneEntity*>& entities);

		static void RemapEntityReferences(const std::unordered_map<uint64_t, uint64_t>& uuid_lookup, SceneEntity& entity);

		static SceneEntity& DeserializeEntityUUIDFromString(Scene& scene, const std::string& str);

		static void SerializeEntityNodeRef(YAML::Emitter& out, const EntityNodeRef& ref);

		static void DeserializeEntityNodeRef(const YAML::Node& node, EntityNodeRef& ref);

		/* 
			Component deserializers 
		*/

		static void DeserializeMeshComp(const YAML::Node& node, SceneEntity& entity);

		static void DeserializePointlightComp(const YAML::Node& node, SceneEntity& entity);

		static void DeserializeSpotlightComp(const YAML::Node& node, SceneEntity& entity);

		static void DeserializeCameraComp(const YAML::Node& node, SceneEntity& entity);

		static void DeserializeScriptComp(const YAML::Node& node, SceneEntity& entity);

		static void DeserializePhysicsComp(const YAML::Node& node, SceneEntity& entity);

		static void DeserializeAudioComp(const YAML::Node& node, SceneEntity& entity);

		static void DeserializeParticleEmitterComp(const YAML::Node& node, SceneEntity& entity);

		static void DeserializeParticleBufferComp(const YAML::Node& node, SceneEntity& entity);

		static VehicleComponent* DeserializeVehicleComp(const YAML::Node& node, SceneEntity& entity);

		static void DeserializeTransformComp(const YAML::Node& node, SceneEntity& entity);

		static void DeserializeCharacterControllerComp(const YAML::Node& node, SceneEntity& entity);

		static void DeserializeJointComp(const YAML::Node& node, SceneEntity& entity);

		// This resolves the EntityNodeRef's in the joint component and actually connects the joint
		// This has to occur after the full scene tree has been deserialized for the EntityNodeRef's to navigate properly
		static void ConnectJointComp(Scene& scene, JointComponent& comp);


		template <typename T>
		static void SerializeBinary(const std::string& filepath, T& data) {
			std::ofstream s{ filepath, s.binary | s.trunc | s.out };
			if (!s.is_open()) {
				ORNG_CORE_ERROR("Binary serialization error: Cannot open {0} for writing", filepath);
				return;
			}
			bitsery::Serializer<bitsery::OutputBufferedStreamAdapter> ser{ s };

			ser.object(data);
			// flush to writer
			ser.adapter().flush();
			s.close();
		}

		template <typename T>
		static void DeserializeBinary(const std::string& filepath, T& data) {
			std::ifstream s{ filepath, std::ios::binary };
			if (!s.is_open()) {
				ORNG_CORE_ERROR("Deserialization error: Cannot open {0} for reading", filepath);
				return;
			}

			// Use buffered stream adapter
			bitsery::Deserializer<bitsery::InputStreamAdapter> des{ s };
			des.object(data);
		}



		template <typename S>
		void serialize(S& s, UUID<uint64_t>& o) {
			s.value8b(o.m_uuid);
		}

	private:

	};
}