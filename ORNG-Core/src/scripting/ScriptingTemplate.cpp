#include "./includes/ScriptAPI.h"

/*
	Apart from entity/scene methods, only use methods inside the "ScriptInterface" namespace
	Attempting to use any singleton classes from the ORNG namespace will result in either undefined behaviour or runtime crashes
*/

extern "C" {
	using namespace ORNG;

	__declspec(dllexport) void OnCreate(ORNG::SceneEntity* p_entity) {

	}

	__declspec(dllexport) void OnUpdate(ORNG::SceneEntity* p_entity) {

	}

	__declspec(dllexport) void OnDestroy(ORNG::SceneEntity* p_entity) {

	}

	__declspec(dllexport) void OnCollision(ORNG::SceneEntity* p_this, ORNG::SceneEntity* p_other) {

	}

}