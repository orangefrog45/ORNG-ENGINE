#pragma once
#include "scene/SceneEntity.h"
#include <yaml-cpp/yaml.h>

namespace ORNG {
	class Scene;

	class SceneSerializer {
	public:
		static void SerializeScene(const Scene& scene, const std::string& filepath);
		static bool DeserializeScene(Scene& scene, const std::string& filepath);
		static void SerializeEntity(SceneEntity& entity, YAML::Emitter& out);
		static void DeserializeEntity(Scene& scene, YAML::iterator::value_type entity_node, SceneEntity& entity);
	private:
	};
}