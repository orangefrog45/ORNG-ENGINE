#ifndef SCRIPTCOMPONENT_H
#define SCRIPTCOMPONENT_H

#include "components/Component.h"
#include "scripting/ScriptShared.h"

namespace ORNG {
	class ScriptComponent : public Component {
		friend class EditorLayer;
		friend class SceneSerializer;
	public:
		explicit ScriptComponent(SceneEntity* p_entity) : Component(p_entity) {
			if (p_instance)
				p_symbols->DestroyInstance(p_instance);
		};

		void SetSymbols(const ScriptSymbols* t_symbols);

		const ScriptSymbols* GetSymbols() {
			return p_symbols;
		}

		ScriptBase* p_instance = nullptr;

	private:
		const ScriptSymbols* p_symbols = nullptr;
	};
}

#endif