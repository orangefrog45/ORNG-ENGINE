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
		static void SerializeScene(const Scene& scene, const std::string& filepath);
		static bool DeserializeScene(Scene& scene, const std::string& filepath);
		static void SerializeEntity(SceneEntity& entity, YAML::Emitter& out);
		// Entity argument is the entity that the data will be loaded into
		static void DeserializeEntity(Scene& scene, YAML::Node& entity_node, SceneEntity& entity);

		static void SerializeMeshAssetBinary(const std::string& filepath, MeshAsset& data);
		static void DeserializeMeshAssetBinary(const std::string& filepath, MeshAsset& data);

		static std::string SerializeEntityIntoString(SceneEntity& entity);
		// Entity argument is the entity that the data will be loaded into
		static void DeserializeEntityFromString(Scene& scene, const std::string& str, SceneEntity& entity);


		template <typename S>
		void serialize(S& s, UUID& o) {
			s.value8b(o.m_uuid);
		}

	};
}