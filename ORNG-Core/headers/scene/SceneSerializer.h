#pragma once
#include "util/UUID.h"

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


	class SceneSerializer {
	public:
		// Output is either the filepath to write to or a string to be written to, if write_to_string is true then the string will be written to, no files
		static void SerializeScene(const Scene& scene, std::string& output, bool write_to_string = false);

		// Produces a .h file with UUID values for each named entity and asset, used in scripts
		static void SerializeSceneUUIDs(const Scene& scene, std::string& output);


		// Deserializes from filepath at "input" if input_is_filepath = true, else deserializes from the string itself assuming it contains valid yaml data
		static bool DeserializeScene(Scene& scene, const std::string& input, bool load_env_map, bool input_is_filepath = true);

		static void SerializeEntity(SceneEntity& entity, YAML::Emitter& out);
		// Entity argument is the entity that the data will be loaded into
		static void DeserializeEntity(Scene& scene, YAML::Node& entity_node, SceneEntity& entity);


		static std::string SerializeEntityIntoString(SceneEntity& entity);
		// Entity argument is the entity that the data will be loaded into
		static void DeserializeEntityFromString(Scene& scene, const std::string& str, SceneEntity& entity);


		template <typename S>
		void serialize(S& s, UUID& o) {
			s.value8b(o.m_uuid);
		}

	private:

	};
}