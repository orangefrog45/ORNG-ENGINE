#include "EditorEventStack.h"
#include "scene/SceneSerializer.h"
#include "scene/Scene.h"

namespace ORNG {
	void EditorEventStack::Undo() {
		if (m_events.empty() || m_active_index == -1)
			return;

		auto& e = m_events[m_active_index];

		if (m_active_index > -1)
			m_active_index--;


		std::vector<SceneEntity*> entities;

		if (e.event_type == ENTITY_CREATE || e.event_type == TRANSFORM_UPDATE) {
			for (int i = 0; i < e.affected_entities.size(); i++) {
				entities.push_back((*mp_scene_context)->GetEntity(e.affected_entities[i]));
			}
		}
		else if (e.event_type == ENTITY_DELETE) {
			for (int i = 0; i < e.affected_entities.size(); i++) {
				entities.push_back(&(*mp_scene_context)->CreateEntity("UNDONE ENTITY", e.affected_entities[i]));
			}
		}

		if (e.event_type == ENTITY_CREATE) {
			for (int i = 0; i < entities.size(); i++) {
				e.serialized_entities_after.push_back(SceneSerializer::SerializeEntityIntoString(*entities[i]));
				(*mp_scene_context)->DeleteEntity(entities[i]);
			}
		}
		else {
			for (int i = 0; i < entities.size(); i++) {
				e.serialized_entities_after.push_back(SceneSerializer::SerializeEntityIntoString(*entities[i]));
				SceneSerializer::DeserializeEntityFromString(**mp_scene_context, e.serialized_entities_before[i], *entities[i]);
			}
		}
	}

	void EditorEventStack::Redo() {
		if (m_events.empty() || m_active_index == m_events.size() - 1)
			return;

		m_active_index++;

		auto& e = m_events[m_active_index];

		std::vector<SceneEntity*> entities;

		if (e.event_type == ENTITY_CREATE) {
			for (int i = 0; i < e.affected_entities.size(); i++) {
				auto& ent = (*mp_scene_context)->CreateEntity("new", e.affected_entities[i]);
				entities.push_back(&ent);
			}
		}
		else if (e.event_type == ENTITY_DELETE) {
			for (int i = 0; i < e.affected_entities.size(); i++) {
				(*mp_scene_context)->DeleteEntity((*mp_scene_context)->GetEntity(e.affected_entities[i]));
			}
		}
		else if (e.event_type == TRANSFORM_UPDATE) {
			for (int i = 0; i < e.affected_entities.size(); i++) {
				entities.push_back((*mp_scene_context)->GetEntity(e.affected_entities[i]));
			}
		}


		if (e.event_type == ENTITY_CREATE || e.event_type == TRANSFORM_UPDATE) {
			for (int i = 0; i < entities.size(); i++) {
				SceneSerializer::DeserializeEntityFromString(**mp_scene_context, e.serialized_entities_after[i], *entities[i]);
			}
		}
	}
}