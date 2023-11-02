#include "./includes/ScriptAPI.h"

/*
	Apart from entity/scene methods, only use methods inside the "ScriptInterface" namespace
	Attempting to use any singleton classes from the ORNG namespace will result in either undefined behaviour or runtime crashes
*/

extern "C" {
	using namespace ORNG;

	__declspec(dllexport) void OnCreate(ORNG::SceneEntity* p_entity) {
		auto* p_data = p_entity->AddComponent<DataComponent>();
		p_data->data["startTime"] = ScriptInterface::FrameTiming::GetElapsedTime();
	}

	__declspec(dllexport) void OnUpdate(ORNG::SceneEntity* p_entity) {
		auto* p_data = p_entity->GetComponent<DataComponent>();
		float st = p_data->Get<float>("startTime");
		if (abs(st - ScriptInterface::FrameTiming::GetElapsedTime()) > 5000.0) {
			ScriptInterface::Scene::DeleteEntity(p_entity);
			return;
		}

	}

	__declspec(dllexport) void OnDestroy(ORNG::SceneEntity* p_entity) {

	}

	__declspec(dllexport) void OnCollision(ORNG::SceneEntity* p_this, ORNG::SceneEntity* p_other) {

	}

}