#pragma once

namespace YAML {
	class Emitter;
	class Node;
}


namespace ORNG {
	class Scene;
	class SceneEntity;
	class VAO;
	struct VertexData3D;


	class SceneSerializer {
	public:
		static void SerializeScene(const Scene& scene, const std::string& filepath);
		static bool DeserializeScene(Scene& scene, const std::string& filepath);
		static void SerializeEntity(SceneEntity& entity, YAML::Emitter& out);
		// Entity argument is the entity that the data will be loaded into
		static void DeserializeEntity(Scene& scene, YAML::Node& entity_node, SceneEntity& entity);

		static void SerializeVertexDataBinary(const std::string& filepath, const VertexData3D& data);

		static std::string SerializeEntityIntoString(SceneEntity& entity);
		// Entity argument is the entity that the data will be loaded into
		static void DeserializeEntityFromString(Scene& scene, const std::string& str, SceneEntity& entity);
	};
}