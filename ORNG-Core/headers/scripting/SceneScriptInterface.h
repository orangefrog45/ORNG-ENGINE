#ifndef SCENE_SCRIPT_INTERFACE_H
#define SCENE_SCRIPT_INTERFACE_H
#include "ScriptShared.h"
namespace ORNG {
	class SceneEntity;
}

namespace physx {
	class PxGeometry;
}
namespace ScriptInterface {
	namespace World {
		inline static std::function<ORNG::SceneEntity& (const std::string&)> CreateEntity = nullptr;
		inline static std::function<ORNG::SceneEntity& (uint64_t)> GetEntity = nullptr;
		inline static std::function<ORNG::SceneEntity& (const std::string&)> GetEntityByName = nullptr;
		inline static std::function<void(ORNG::SceneEntity*)> DeleteEntity = nullptr;
		inline static std::function<ORNG::SceneEntity& (ORNG::SceneEntity&)> DuplicateEntity = nullptr;
		inline static std::function<ORNG::SceneEntity& (const std::string&)> InstantiatePrefab = nullptr;
		inline static std::function<ORNG::RaycastResults(glm::vec3 origin, glm::vec3 unit_dir, float max_distance)> Raycast = nullptr;
		inline static std::function<ORNG::OverlapQueryResults(physx::PxGeometry&, glm::vec3 origin)> OverlapQuery = nullptr;
	}
}
#endif