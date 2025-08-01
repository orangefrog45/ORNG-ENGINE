#pragma once
#include "components/systems/ComponentSystem.h"
#include "components/ScriptComponent.h"
#include "scene/Scene.h"

namespace ORNG {
	class ScriptSystem : public ComponentSystem {
	public:
		explicit ScriptSystem(Scene* p_scene) : ComponentSystem(p_scene) {};
		virtual ~ScriptSystem() = default;

		void OnLoad() final;

		void OnUpdate() final;

		void OnUnload() final {
			m_script_destroy_connection.release();
		}

		inline static constexpr uint64_t GetSystemUUID() { return 28374646389289; }
	private:

		inline static void OnScriptDestroy(entt::registry& registry, entt::entity entity) {
			auto& script = registry.get<ScriptComponent>(entity);
			if (script.GetSymbols()->loaded)
				if (script.p_instance) script.GetSymbols()->DestroyInstance(script.p_instance);
		}

		entt::connection m_script_destroy_connection;
	};
}