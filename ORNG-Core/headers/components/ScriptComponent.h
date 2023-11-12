#ifndef SCRIPTCOMPONENT_H
#define SCRIPTCOMPONENT_H

#include "components/Component.h"
#include "scripting/ScriptingEngine.h" // For ScriptSymbols
#include "scripting/ScriptShared.h"

namespace ORNG {

	class SceneEntity;

	struct ScriptComponent : public Component {
		friend class EditorLayer;
		friend class SceneSerializer;
		explicit ScriptComponent(SceneEntity* p_entity) : Component(p_entity) {};

		void SetSymbols(const ScriptSymbols* t_symbols) {
			p_symbols = t_symbols;
			script_filepath = t_symbols->script_path;
		}


	private:
		const ScriptSymbols* p_symbols = nullptr;
		ScriptBase* p_instance = nullptr;
		std::string script_filepath = "";
	};
}

#endif