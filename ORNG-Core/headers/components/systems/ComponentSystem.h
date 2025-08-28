#pragma once
#include "scene/Scene.h"

namespace ORNG {
	// Classes that inherit from this class MUST implement the function "static constexpr uint64_t GetSystemUUID()" which returns a UUID unique to that class
	class ComponentSystem {
	public:
		explicit ComponentSystem(Scene* p_scene) : mp_scene(p_scene) {}
		// Dispatches event attached to single component, used for connections with entt::registry::on_construct etc
		template<std::derived_from<Component> T>
		static void DispatchComponentEvent(entt::registry& registry, entt::entity entity, Events::ECS_EventType type) {
			Events::ECS_Event<T> e_event{ type, &registry.get<T>(entity) };
			Events::EventManager::DispatchEvent(e_event);
		}

		virtual ~ComponentSystem() = default;

		virtual void OnUpdate() {}

		virtual void OnLoad() {}

		virtual void OnUnload() {}

		inline uint64_t GetSceneUUID() const { return mp_scene->GetStaticUUID(); }
	protected:
		Scene* mp_scene = nullptr;
	};
}
