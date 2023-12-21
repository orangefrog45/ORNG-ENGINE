#include "./includes/ScriptAPI.h"

/*
	Apart from entity/scene methods, only use methods inside the "ScriptInterface" namespace
	Attempting to use any singleton classes from the ORNG namespace will result in either undefined behaviour or runtime crashes
*/

extern "C" {
	using namespace ORNG;
	using namespace ScriptInterface;

	class ScriptClassExample : public ScriptBase {
	public:
		ScriptClassExample() { O_CONSTRUCTOR; }

		void OnUpdate() override {
			auto& ship = ScriptInterface::Scene::GetEntity(ScriptInterface::Scene::Entities::Ship);
			auto* p_ship_transform = ship.GetComponent<TransformComponent>();
			auto* p_transform = p_entity->GetComponent<TransformComponent>();

			appearance_chance_cooldown -= ScriptInterface::FrameTiming::GetDeltaTime();

			auto r = rand() % 2;
			if (r == 1 && appearance_chance_cooldown < 0) {
				appearance_chance_cooldown = 2000;

				p_transform->SetPosition(p_ship_transform->GetPosition() - p_ship_transform->forward * 150.f);
				p_transform->LookAt(p_transform->GetPosition() + p_ship_transform->right);
				velocity = p_ship_transform->forward * 10.f;
			}

			p_transform->SetPosition(p_transform->GetPosition() + velocity * FrameTiming::GetDeltaTime() * 0.001f);
		}

	private:
		// Not accessible through other scripts
		int appearance_chance_cooldown = 2000;

		glm::vec3 velocity{ 0, 0, 0 };
	};
}

// ORNG_CLASS must be defined as exported class
#define ORNG_CLASS ScriptClassExample

#include "includes/ScriptInstancer.h"
#include "includes/ScriptAPIImpl.h"