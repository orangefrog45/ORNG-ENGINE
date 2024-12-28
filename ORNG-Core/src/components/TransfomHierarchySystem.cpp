#include "pch/pch.h"
#include "components/ComponentSystems.h"
#include "scene/SceneEntity.h"

namespace ORNG {

	void TransformHierarchySystem::UpdateChildTransforms(const Events::ECS_Event<TransformComponent>& t_event) {
		auto* p_relationship_comp = t_event.p_component->GetEntity()->GetComponent<RelationshipComponent>();
		entt::entity current_entity = p_relationship_comp->first;

		auto& reg = mp_scene->GetRegistry();

		for (int i = 0; i < p_relationship_comp->num_children; i++) {
			auto& transform = reg.get<TransformComponent>(current_entity);

			if (transform.m_is_absolute)
				continue;

			transform.RebuildMatrix(static_cast<TransformComponent::UpdateType>(t_event.sub_event_type));
			current_entity = reg.get<RelationshipComponent>(current_entity).next;
		}
	}

	void TransformHierarchySystem::OnLoad() {
		// On transform update event, update all child transforms
		m_transform_event_listener.OnEvent = [this](const Events::ECS_Event<TransformComponent>& t_event) {
			[[likely]] if (t_event.event_type == Events::ECS_EventType::COMP_UPDATED) {
				UpdateChildTransforms(t_event);
			}
		};

		m_transform_event_listener.scene_id = GetSceneUUID();
		Events::EventManager::RegisterListener(m_transform_event_listener);
	}
}