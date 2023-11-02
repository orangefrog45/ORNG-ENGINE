#include "./includes/ScriptAPI.h"
#define PI 3.141592
/*
	Apart from entity/scene methods, only use methods inside the "ScriptInterface" namespace
	Attempting to use any singleton classes from the ORNG namespace will result in either undefined behaviour or runtime crashes
*/

extern "C" {
	using namespace ORNG;

	__declspec(dllexport) void OnCreate(ORNG::SceneEntity* p_entity) {
		auto* p_data = p_entity->AddComponent<DataComponent>();
		p_data->data["startPos"] = glm::vec3{ 0, 0, 0 }; // Default, should be set externally
		auto* p_audio = p_entity->AddComponent<AudioComponent>();
		p_audio->SetVolume(1.0);
		p_audio->SetMinMaxRange(0.1, 10000);
		//p_audio->Play(ScriptInterface::Scene::Sounds::blaster_2_81267_mp3);
	}

	__declspec(dllexport) void OnUpdate(ORNG::SceneEntity* p_entity) {
		auto* p_transform = p_entity->GetComponent<TransformComponent>();
		auto* p_data = p_entity->GetComponent<DataComponent>();

		glm::vec3 abs_pos = p_transform->GetAbsoluteTransforms()[0];
		p_transform->SetPosition(abs_pos + p_transform->forward * ScriptInterface::FrameTiming::GetDeltaTime() * 0.1f);

		if (glm::length(p_data->Get<glm::vec3>("startPos") - abs_pos) > 1000.0)
			ScriptInterface::Scene::DeleteEntity(p_entity);
	}

	__declspec(dllexport) void OnDestroy(ORNG::SceneEntity* p_entity) {
	}

	__declspec(dllexport) void OnCollision(ORNG::SceneEntity* p_this, ORNG::SceneEntity* p_other) {
		int num_debris = 8;
		for (int i = 0; i < num_debris; i++) {
			auto& ent = ScriptInterface::Scene::InstantiatePrefab(ScriptInterface::Scene::Prefabs::OrangeDebris);
			ent.GetComponent<TransformComponent>()->SetPosition(p_this->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0]);
			auto* p_phys_comp = ent.GetComponent<PhysicsComponent>();
			p_phys_comp->AddForce(glm::vec3(sinf((i / num_debris) * PI * 2.f) * 5.f, (rand() % 10) - 5, cosf((i / num_debris) * PI * 2.f) * 5.f));
		}
		ScriptInterface::Scene::DeleteEntity(p_this);
	}
}