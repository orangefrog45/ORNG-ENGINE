
#ifndef SCENE_SCRIPT_INTERFACE_H
#define SCENE_SCRIPT_INTERFACE_H
namespace ORNG {
	class SceneEntity;
}
namespace ScriptInterface {
	namespace Scene {
		inline static std::function<ORNG::SceneEntity& (const std::string&)> CreateEntity = nullptr;
		inline static std::function<void(ORNG::SceneEntity*)> DeleteEntity = nullptr;
		inline static std::function<ORNG::SceneEntity& (ORNG::SceneEntity&)> DuplicateEntity = nullptr;
		inline static std::function<ORNG::SceneEntity& (const std::string&)> InstantiatePrefab = nullptr;
	}

}
#endif