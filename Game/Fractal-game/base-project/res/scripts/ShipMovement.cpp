#include "./includes/ScriptAPI.h"

/*
	Apart from entity/scene methods, only use methods inside the "ScriptInterface" namespace
	Attempting to use any singleton classes from the ORNG namespace will result in either undefined behaviour or runtime crashes
*/

extern "C" {
	using namespace ORNG;

	class ScriptClassExample : public ScriptBase {
	public:
		ScriptClassExample() { O_CONSTRUCTOR; }

		void OnCreate() override {
			ScriptInterface::Input::SetMouseVisible(false);
			auto* p_audio = ScriptInterface::Scene::GetEntity(ScriptInterface::Scene::Entities::Cam).GetComponent<AudioComponent>();
			p_audio->Play();
			p_audio->SetLooped(true);
		}

		void OnUpdate() override {
			auto* p_transform = p_entity->GetComponent<TransformComponent>();

			float dt = ScriptInterface::FrameTiming::GetDeltaTime() * 0.001;

			thrust += ScriptInterface::Input::IsKeyDown('w') * dt * 8.0;
			thrust -= thrust * ScriptInterface::Input::IsKeyDown('s') * dt * 0.001f;

			glm::vec2 mouse_delta = ScriptInterface::Input::GetMouseDelta();
			glm::quat q = glm::angleAxis(-mouse_delta.x * dt * 0.1f, p_transform->up);
			q = glm::angleAxis(mouse_delta.y * dt * 0.1f, p_transform->right) * q;
			q = normalize(q);

			orientation = glm::normalize(q * orientation);

			auto look_at_pos = p_transform->GetPosition() + orientation * glm::vec3(0, 0, -1);


			p_transform->SetPosition(p_transform->GetPosition() - p_transform->forward * thrust * dt);
			p_transform->LookAt(look_at_pos);

			if (ScriptInterface::Input::IsKeyPressed('f')) {
				auto& ent = ScriptInterface::Scene::InstantiatePrefab(ScriptInterface::Scene::Prefabs::Flare);
				ent.GetComponent<ScriptComponent>()->p_instance->Get<glm::vec3>("velocity") = -p_transform->forward * 20.f;
				ent.GetComponent<TransformComponent>()->SetPosition(p_transform->GetPosition() - p_transform->forward);
				ent.GetComponent<TransformComponent>()->LookAt(p_transform->GetPosition() - p_transform->forward * 2.f);
			}

			auto* p_audio = ScriptInterface::Scene::GetEntity(ScriptInterface::Scene::Entities::Cam).GetComponent<AudioComponent>();
			p_audio->SetPitch(0.45f + thrust * 0.015f);
			thrust = thrust - thrust * glm::min(dt, 1.f);
		}

	private:
		glm::quat orientation = glm::identity<glm::quat>();

		O_PROPERTY float thrust = 0.f;

	};
}

// ORNG_CLASS must be defined as exported class
#define ORNG_CLASS ScriptClassExample

#include "includes/ScriptInstancer.h"
#include "includes/ScriptAPIImpl.h"