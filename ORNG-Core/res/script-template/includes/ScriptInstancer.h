#include "ScriptShared.h"

extern "C" {
#ifndef ORNG_CLASS
#error Scripts must export a class, define ORNG_CLASS as your class type
#else
	inline static std::function<ORNG::SceneEntity& (const std::string&)> CreateEntity = nullptr;
	inline static std::function<ORNG::SceneEntity& (uint64_t)> GetEntity = nullptr;
	inline static std::function<void(ORNG::SceneEntity*)> DeleteEntity = nullptr;
	inline static std::function<ORNG::SceneEntity& (ORNG::SceneEntity&)> DuplicateEntity = nullptr;
	inline static std::function<ORNG::SceneEntity& (const std::string&)> InstantiatePrefab = nullptr;
	inline static std::function<ORNG::RaycastResults(glm::vec3 origin, glm::vec3 unit_dir, float max_distance)> Raycast = nullptr;

	__declspec(dllexport) ORNG::ScriptBase* CreateInstance() {
		return static_cast<ORNG::ScriptBase*>(new ORNG_CLASS());
	}

	__declspec(dllexport) void DestroyInstance(ORNG::ScriptBase* p_instance) {
		delete static_cast<ORNG_CLASS*>(p_instance);
	}

#endif
}