#include "pch/pch.h"
#include "components/systems/TransformHierarchySystem.h"
#include "scene/SceneEntity.h"
#include "scene/Scene.h"

namespace ORNG {

	void TransformHierarchySystem::UpdateChildTransforms(entt::entity parent_handle, TransformComponent::UpdateType type) {
		auto* p_relationship_comp = mp_scene->GetRegistry().try_get<RelationshipComponent>(parent_handle);
		if (!p_relationship_comp)
			return;

		entt::entity current_entity = p_relationship_comp->first;
		auto& reg = mp_scene->GetRegistry();

		for (int i = 0; i < p_relationship_comp->num_children; i++) {
			auto& transform = reg.get<TransformComponent>(current_entity);
			auto& rel = reg.get<RelationshipComponent>(current_entity);

			if (transform.m_is_absolute) {
				UpdateChildTransforms(current_entity, type);
			} else {
				transform.RebuildMatrix(type);
			}

			current_entity = rel.next;
		}
	}

	void TransformHierarchySystem::OnLoad() {
		// On transform update event, update all child transforms
		m_transform_event_listener.OnEvent = [this](const Events::ECS_Event<TransformComponent>& t_event) {
			[[likely]] if (t_event.event_type == Events::ECS_EventType::COMP_UPDATED) {
				UpdateChildTransforms(t_event.p_component->GetEntity()->GetEnttHandle(), static_cast<TransformComponent::UpdateType>(t_event.sub_event_type));
			}
		};

		m_transform_event_listener.scene_id = GetSceneUUID();
		Events::EventManager::RegisterListener(m_transform_event_listener);
	}
}
