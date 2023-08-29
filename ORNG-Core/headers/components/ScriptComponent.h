#ifndef SCRIPTCOMPONENT_H
#define SCRIPTCOMPONENT_H

#include "components/Component.h"
namespace ORNG {

	class SceneEntity;

	struct ScriptComponent : public Component {
		friend class EditorLayer;
		explicit ScriptComponent(SceneEntity* p_entity) : Component(p_entity) {};

		std::function<void(SceneEntity*, Scene*)> OnCreate = [](SceneEntity*, Scene*) {};
		std::function<void(SceneEntity*, Scene*)> OnUpdate = [](SceneEntity*, Scene*) {};
		std::function<void(SceneEntity*, Scene*)> OnDestroy = [](SceneEntity*, Scene*) {};
	private:
		std::string script_filepath = "";
	};
}

#endif