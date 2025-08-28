#include "EditorEventStack.h"
#include "scene/SceneSerializer.h"
#include "scene/Scene.h"
#include "scene/SceneEntity.h"

namespace ORNG {
	void EditorEventStack::Undo() {
		if (m_events.empty() || static_cast<size_t>(m_active_index) == m_events.size())
			return;

		ASSERT(m_active_index >= 0);

		auto& e = m_events[static_cast<unsigned>(m_active_index)];

		if (static_cast<size_t>(m_active_index) < m_events.size())
			m_active_index++;

		if (e.event_type == ENTITY_CREATE) {
			for (size_t i = 0; i < e.affected_entities.size(); i++) {

				auto& ent = *(*mp_scene_context)->GetEntity(e.affected_entities[i]);

				RemoveIfContains(*mp_editor_selected_entities, ent.GetUUID());
				(*mp_scene_context)->DeleteEntity(&ent);
			}
		}
		else if (e.event_type == TRANSFORM_UPDATE) {
			bool empty_serialized_after = e.serialized_entities_after.empty();

			for (size_t i = 0; i < e.affected_entities.size(); i++) {
				auto& ent = *(*mp_scene_context)->GetEntity(e.affected_entities[i]);

				if (empty_serialized_after)
					e.serialized_entities_after.push_back(SceneSerializer::SerializeEntityIntoString(ent));

				SceneSerializer::DeserializeEntityFromString(**mp_scene_context, e.serialized_entities_before[i], ent);
			}

			for (size_t i = 0; i < e.affected_entities.size(); i++) {
				Events::EventManager::DispatchEvent(EntitySerializationEvent{(*mp_scene_context)->GetEntity(e.affected_entities[i])});
			}
		}
		else if (e.event_type == ENTITY_DELETE) {
			for (size_t i = 0; i < e.affected_entities.size(); i++) {
				SceneSerializer::DeserializeEntityUUIDFromString(**mp_scene_context, e.serialized_entities_before[i]);
			}

			bool empty_serialized_after = e.serialized_entities_after.empty();

			for (size_t i = 0; i < e.affected_entities.size(); i++) {
				auto& ent = *(*mp_scene_context)->GetEntity(e.affected_entities[i]);
				SceneSerializer::DeserializeEntityFromString(**mp_scene_context, e.serialized_entities_before[i], ent);

				if (empty_serialized_after)
					e.serialized_entities_after.push_back(std::to_string(ent.GetUUID()));
			}

			for (size_t i = 0; i < e.affected_entities.size(); i++) {
				Events::EventManager::DispatchEvent(EntitySerializationEvent{(*mp_scene_context)->GetEntity(e.affected_entities[i])});
			}
		}
	}
	void EditorEventStack::Redo() {
		if (m_events.empty() || m_active_index == 0)
			return;

		m_active_index--;
		ASSERT(m_active_index >= 0);

		auto& e = m_events[static_cast<unsigned>(m_active_index)];

		if (e.event_type == ENTITY_CREATE) {
			for (size_t i = 0; i < e.affected_entities.size(); i++) {
				SceneSerializer::DeserializeEntityUUIDFromString(**mp_scene_context, e.serialized_entities_before[i]);
			}

			for (size_t i = 0; i < e.affected_entities.size(); i++) {
				auto& ent = *(*mp_scene_context)->GetEntity(e.affected_entities[i]);
				SceneSerializer::DeserializeEntityFromString(**mp_scene_context, e.serialized_entities_before[i], ent);
			}

			for (size_t i = 0; i < e.affected_entities.size(); i++) {
				Events::EventManager::DispatchEvent(EntitySerializationEvent{(*mp_scene_context)->GetEntity(e.affected_entities[i])});
			}
		}
		else if (e.event_type == ENTITY_DELETE) {
			for (size_t i = 0; i < e.serialized_entities_after.size(); i++) {
				auto& ent = *(*mp_scene_context)->GetEntity(std::stoull(e.serialized_entities_after[i]));
				RemoveIfContains(*mp_editor_selected_entities, ent.GetUUID());
				(*mp_scene_context)->DeleteEntity(&ent);
			}
		}
		else if (e.event_type == TRANSFORM_UPDATE) {
			for (size_t i = 0; i < e.affected_entities.size(); i++) {
				SceneSerializer::DeserializeEntityFromString(**mp_scene_context, e.serialized_entities_after[i], *(*mp_scene_context)->GetEntity(e.affected_entities[i]));
			}

			for (size_t i = 0; i < e.affected_entities.size(); i++) {
				Events::EventManager::DispatchEvent(EntitySerializationEvent{(*mp_scene_context)->GetEntity(e.affected_entities[i])});
			}
		}
	}

	void EditorEventStack::PushEvent(const EditorEntityEvent& e) {

		if (m_active_index != 0)
			m_events.erase(m_events.begin(), m_events.begin() + glm::max(m_active_index, 0));

		m_events.push_front(e);

		if (m_events.size() > MAX_EVENT_HISTORY)
			m_events.erase(m_events.begin());

		m_active_index = 0;

		constexpr unsigned MAX_STORED_EVENTS = 50;
		if (m_events.size() > MAX_STORED_EVENTS) {
			m_events.pop_back();
		}
	};

	void EditorEventStack::Clear() {
		m_events.clear();
		m_active_index = -1;
	}
}
