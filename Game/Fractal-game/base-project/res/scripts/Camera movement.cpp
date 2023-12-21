#include "./includes/ScriptAPI.h"
#include <iostream>
/*
	Apart from entity/scene methods, only use methods inside the "ScriptInterface" namespace
	Attempting to use any singleton classes from the ORNG namespace will result in either undefined behaviour or runtime crashes
*/

extern "C" {
	using namespace ORNG;

	class ScriptClassExample : public ScriptBase {
	public:
		ScriptClassExample() { O_CONSTRUCTOR; }

		void OnUpdate() override {
			float thrust = glm::max(ScriptInterface::Scene::GetEntity(ScriptInterface::Scene::Entities::Ship).GetComponent<ScriptComponent>()->p_instance->Get<float>("thrust"), 1.f);
			std::cout << thrust<< "\n";
			auto* p_transform = p_entity->GetComponent<TransformComponent>();
			p_transform->SetOrientation(p_transform->GetOrientation() + glm::vec3(rand() % (int)thrust, rand() % (int)thrust, rand() % (int)thrust) * ScriptInterface::FrameTiming::GetDeltaTime() * 0.1f);
		}

	private:
	};
}

// ORNG_CLASS must be defined as exported class
#define ORNG_CLASS ScriptClassExample

#include "includes/ScriptInstancer.h"
#include "includes/ScriptAPIImpl.h"