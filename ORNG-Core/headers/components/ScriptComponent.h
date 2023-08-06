#pragma once
//#include <functional>
#include "components/Component.h"
namespace ORNG {

	class SceneEntity;

	class ScriptComponent : public Component {
	public:
		explicit ScriptComponent(SceneEntity* p_entity) : Component(p_entity) {};

		std::function<void()> OnUpdate = nullptr;
	private:
	};
}