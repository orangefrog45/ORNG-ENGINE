#pragma once
//#include <functional>
#include "components/Component.h"
namespace ORNG {

	class SceneEntity;

	class ScriptComponent : public Component {
	public:
		explicit ScriptComponent(unsigned long entity_id) : Component(entity_id) {};

		std::function<void()> OnUpdate = nullptr;
	private:
	};
}