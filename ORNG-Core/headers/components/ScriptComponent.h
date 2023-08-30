#ifndef SCRIPTCOMPONENT_H
#define SCRIPTCOMPONENT_H

#include "components/Component.h"
#include "scripting/ScriptingEngine.h" // For ScriptSymbols

namespace ORNG {

	class SceneEntity;

	struct ScriptComponent : public Component {
		friend class EditorLayer;
		friend class SceneSerializer;
		explicit ScriptComponent(SceneEntity* p_entity) : Component(p_entity) {};

		void SetSymbols(const ScriptSymbols* p_symbols) {
			OnCreate = p_symbols->OnCreate;
			OnUpdate = p_symbols->OnUpdate;
			OnDestroy = p_symbols->OnDestroy;
			OnCollision = p_symbols->OnCollision;
			script_filepath = p_symbols->script_path;
		}

		std::function<void(SceneEntity*, Scene*)> OnCreate = [](SceneEntity*, Scene*) {};
		std::function<void(SceneEntity*, Scene*)> OnUpdate = [](SceneEntity*, Scene*) {};
		std::function<void(SceneEntity*, Scene*)> OnDestroy = [](SceneEntity*, Scene*) {};
		std::function<void(SceneEntity*, SceneEntity*, Scene*)> OnCollision = [](SceneEntity*, SceneEntity*, Scene*) {};
	private:
		std::string script_filepath = "";
	};
}

#endif