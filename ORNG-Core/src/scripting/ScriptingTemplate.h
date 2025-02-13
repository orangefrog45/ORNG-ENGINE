#pragma once
#include "../includes/ScriptAPI.h"

extern "C" {
	using namespace ORNG;

	class ScriptClassExample : public ScriptBase {
	public:
		ScriptClassExample() = default;

		void OnCreate() override;

		void OnUpdate(float dt) override;

		void OnDestroy() override;
	};
}
