#pragma once

#include "components/systems/ComponentSystem.h"
#include "components/TransformComponent.h"

namespace ORNG {
	class TransformHierarchySystem : public ComponentSystem {
	public:
		explicit TransformHierarchySystem(Scene* p_scene) : ComponentSystem(p_scene) {}
		~TransformHierarchySystem() override = default;

		void OnLoad() override;

		void OnUnload() override {
			Events::EventManager::DeregisterListener(m_transform_event_listener.GetRegisterID());
		}

		inline static constexpr uint64_t GetSystemUUID() { return 934898474626; }

	private:
		void UpdateChildTransforms(const Events::ECS_Event<TransformComponent>&);
		Events::ECS_EventListener<TransformComponent> m_transform_event_listener;
	};
}
