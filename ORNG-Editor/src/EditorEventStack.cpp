#include "EditorEventStack.h"
#include "scene/SceneSerializer.h"
#include "scene/Scene.h"
#include "scene/SceneEntity.h"

namespace ORNG {
	void EditorEventStack::Undo() {
		if (m_events.empty() || m_active_index == m_events.size())
			return;

		auto& e = m_events[m_active_index];

		if (m_active_index < m_events.size())
			m_active_index++;

		if (e.event_type == ENTITY_CREATE) {
			for (int i = 0; i < e.affected_entities.size(); i++) {

				auto& ent = *(*mp_scene_context)->GetEntity(e.affected_entities[i]);

				(*mp_scene_context)->DeleteEntity(&ent);
			}
		}
		else if (e.event_type == TRANSFORM_UPDATE) {
			bool empty_serialized_after = e.serialized_entities_after.empty();

			for (int i = 0; i < e.affected_entities.size(); i++) {
				auto& ent = *(*mp_scene_context)->GetEntity(e.affected_entities[i]);

				if (empty_serialized_after)
					e.serialized_entities_after.push_back(SceneSerializer::SerializeEntityIntoString(ent));

				SceneSerializer::DeserializeEntityFromString(**mp_scene_context, e.serialized_entities_before[i], ent);
			}
		}
		else if (e.event_type == ENTITY_DELETE) {
			for (int i = 0; i < e.affected_entities.size(); i++) {
				SceneSerializer::DeserializeEntityUUIDFromString(**mp_scene_context, e.serialized_entities_before[i]);
			}

			bool empty_serialized_after = e.serialized_entities_after.empty();

			for (int i = 0; i < e.affected_entities.size(); i++) {
				auto& ent = *(*mp_scene_context)->GetEntity(e.affected_entities[i]);
				SceneSerializer::DeserializeEntityFromString(**mp_scene_context, e.serialized_entities_before[i], ent);

				if (empty_serialized_after)
					e.serialized_entities_after.push_back(std::to_string(ent.GetUUID()));
			}
		}
	}
	void EditorEventStack::Redo() {
		if (m_events.empty() || m_active_index == 0)
			return;

		m_active_index--;

		auto& e = m_events[m_active_index];

		if (e.event_type == ENTITY_CREATE) {
			for (int i = 0; i < e.affected_entities.size(); i++) {
				auto& ent = SceneSerializer::DeserializeEntityUUIDFromString(**mp_scene_context, e.serialized_entities_before[i]);
			}

			for (int i = 0; i < e.affected_entities.size(); i++) {
				auto& ent = *(*mp_scene_context)->GetEntity(e.affected_entities[i]);
				SceneSerializer::DeserializeEntityFromString(**mp_scene_context, e.serialized_entities_before[i], ent);
			}
		}
		else if (e.event_type == ENTITY_DELETE) {
			for (int i = 0; i < e.serialized_entities_after.size(); i++) {
				auto& ent = *(*mp_scene_context)->GetEntity(std::stoull(e.serialized_entities_after[i]));
				(*mp_scene_context)->DeleteEntity(&ent);
			}
		}
		else if (e.event_type == TRANSFORM_UPDATE) {
			for (int i = 0; i < e.affected_entities.size(); i++) {
				SceneSerializer::DeserializeEntityFromString(**mp_scene_context, e.serialized_entities_after[i], *(*mp_scene_context)->GetEntity(e.affected_entities[i]));
			}
		}
	}
}