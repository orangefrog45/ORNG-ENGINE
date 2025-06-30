#ifndef SCRIPTCOMPONENT_H
#define SCRIPTCOMPONENT_H

#include "components/Component.h"
#include "scripting/ScriptShared.h"

namespace ORNG {
	class ScriptComponent final : public Component {
		friend class EditorLayer;
		friend class SceneSerializer;
	public:
		explicit ScriptComponent(SceneEntity* p_entity) : Component(p_entity) {}
		~ScriptComponent() override = default;

		void SetSymbols(const ScriptSymbols* t_symbols);

		[[nodiscard]] const ScriptSymbols* GetSymbols() const {
			return p_symbols;
		}

		ScriptBase* p_instance = nullptr;

	private:
		const ScriptSymbols* p_symbols = nullptr;
	};
}

#endif