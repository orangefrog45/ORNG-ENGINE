#pragma once
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include <bitsery/traits/vector.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "util/UUID.h"
#include "events/Events.h"
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
	class MeshComponent;
	struct Prefab;

	struct EntitySerializationEvent : public Events::Event {
		enum class Type {
			SERIALIZING,
			DESERIALIZING,
			POST_DESERIALIZATION,

			// Entity is being duplicated,
			ENTITY_REFERENCE_REMAP,
		} event_type;

		EntitySerializationEvent(SceneEntity* p_ent, YAML::Emitter* p_emitter) : event_type(Type::SERIALIZING), p_entity(p_ent) {
			data.p_emitter = p_emitter;
		}

		EntitySerializationEvent(SceneEntity* p_ent, const YAML::Node* p_node) : event_type(Type::DESERIALIZING), p_entity(p_ent) {
			data.p_node = p_node;
		}

		explicit EntitySerializationEvent(SceneEntity* p_ent) : event_type(Type::POST_DESERIALIZATION), p_entity(p_ent) {
			data.p_emitter = nullptr;
		}

		EntitySerializationEvent(SceneEntity* p_ent, const std::unordered_map<uint64_t, uint64_t>* p_uuid_lookup) : event_type(Type::ENTITY_REFERENCE_REMAP), p_entity(p_ent) {
			data.p_uuid_lookup = p_uuid_lookup;
		}

		SceneEntity* p_entity;

		union {
			// Valid if event_type == SERIALIZING
			YAML::Emitter* p_emitter;

			// Valid if event_type == DESERIALIZING
			const YAML::Node* p_node;

			// Valid if event_type == ENTITY_REFERENCE_REMAP
			// Map of original_entity_uuid -> duplicate_entity_uuid
			const std::unordered_map<uint64_t, uint64_t>* p_uuid_lookup;
		} data;
	};

	struct SceneSerializationEvent : public Events::Event {
		enum class Type {
			SERIALIZING,
			DESERIALIZING
		} event_type;

		explicit SceneSerializationEvent(YAML::Emitter* p_emitter, Scene& _scene) : event_type(Type::SERIALIZING), scene(_scene) {data.p_emitter = p_emitter;}

		explicit SceneSerializationEvent(YAML::Node* p_node, Scene& _scene) : event_type(Type::DESERIALIZING), scene(_scene) {data.p_node = p_node;}

		Scene& scene;

		union {
			// Valid if event_type == SERIALIZING
			YAML::Emitter* p_emitter;

			// Valid if event_type == DESERIALIZING
			// Pointer to the top-level YAML node
			YAML::Node* p_node;
		} data;
	};


	class SceneSerializer {
	public:
		// Output is either the filepath to write to or a string to be written to, if write_to_string is true then the string will be written to, no files
		static void SerializeScene(Scene& scene, std::string& output, bool write_to_string = false);

		// Produces a .h file with UUID values for each named entity and asset, used in scripts
		static void SerializeSceneUUIDs(const std::vector<class SceneAsset*>& scenes, std::string& output);

		// Deserializes from filepath at "input" if input_is_filepath = true, else deserializes from the string itself assuming it contains valid yaml data
		// If "node" is provided, the scene will be deserialized from that instead
		static bool DeserializeScene(Scene& scene, const std::string& input, bool input_is_filepath = true, std::optional<YAML::Node*> node = std::nullopt);

		static void SerializeEntity(SceneEntity& entity, YAML::Emitter& out);

		// Entity argument is the entity that the data will be loaded into
		static void DeserializeEntity(Scene& scene, const YAML::Node& entity_node, SceneEntity& entity, bool ignore_parent = false);

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

		static void DeserializeAudioComp(const YAML::Node& node, SceneEntity& entity);

		static void DeserializeParticleEmitterComp(const YAML::Node& node, SceneEntity& entity);

		static void DeserializeParticleBufferComp(const YAML::Node& node, SceneEntity& entity);

		static void DeserializeTransformComp(const YAML::Node& node, SceneEntity& entity);

		template <typename S>
		void serialize(S& s, UUID<uint64_t>& o) {
			s.value8b(o.m_uuid);
		}

	};
}
