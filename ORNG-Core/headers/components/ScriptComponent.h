#ifndef SCRIPTCOMPONENT_H
#define SCRIPTCOMPONENT_H

#include "components/Component.h"
namespace ORNG {

	class SceneEntity;

	struct ScriptComponent : public Component {
		friend class EditorLayer;
		explicit ScriptComponent(SceneEntity* p_entity) : Component(p_entity) {};

		std::function<void(SceneEntity*)> OnCreate = [](SceneEntity*) {};
		std::function<void(SceneEntity*)> OnUpdate = [](SceneEntity*) {};
		std::function<void(SceneEntity*)> OnDestroy = [](SceneEntity*) {};
	private:
		std::string script_filepath = "";
	};
}

#endif