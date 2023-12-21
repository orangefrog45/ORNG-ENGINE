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

		void OnCreate() override {
			time_start = FrameTiming::GetDeltaTime();
			
		}

		void OnUpdate() override {
			p_entity->GetComponent<PointLightComponent>()->color = glm::vec3(1, 0.1, 0.1) * (0.8f + 0.2f * abs(sinf(ScriptInterface::FrameTiming::GetElapsedTime() * 0.1))) * 3.f;

			float dt = ScriptInterface::FrameTiming::GetDeltaTime();

			velocity = velocity - velocity * dt * 0.00025f;

			auto* p_transform = p_entity->GetComponent<TransformComponent>();

			p_transform->SetPosition(p_transform->GetPosition() + velocity * dt * 0.001f);
		}

	private:
		O_PROPERTY glm::vec3 velocity = { 0, 0, 0 };
		float time_start;
	};
}

// ORNG_CLASS must be defined as exported class
#define ORNG_CLASS ScriptClassExample

#include "includes/ScriptInstancer.h"
#include "includes/ScriptAPIImpl.h"