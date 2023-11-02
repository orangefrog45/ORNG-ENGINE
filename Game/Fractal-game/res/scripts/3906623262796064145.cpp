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
		glm::vec3 pos = p_entity->GetComponent<TransformComponent>()->GetParent()->GetAbsoluteTransforms()[0];
		glm::vec3 target_pos = pos - p_entity->GetComponent<TransformComponent>()->GetParent()->forward * 5.f + glm::vec3(0, 1.5f, 0);
		glm::vec3 old_pos = p_entity->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0];
		glm::vec3 old_to_target = target_pos - old_pos;
		float length = glm::length(old_pos - target_pos);
			//p_entity->GetComponent<TransformComponent>()->SetPosition(target_pos);

		glm::vec3 new_pos = p_entity->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0];
		glm::vec3 dir_vec = glm::normalize(pos - new_pos);
		//p_entity->GetComponent<TransformComponent>()->LookAt(new_pos + (p_entity->GetComponent<TransformComponent>()->forward + dir_vec) * 0.5f);
	}

	__declspec(dllexport) void OnDestroy(ORNG::SceneEntity* p_entity) {

	}

	__declspec(dllexport) void OnCollision(ORNG::SceneEntity* p_this, ORNG::SceneEntity* p_other) {

	}

}