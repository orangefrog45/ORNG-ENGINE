#pragma once



namespace ORNG {
	class Scene;

	enum EditorEntityEventType {
		TRANSFORM_UPDATE,
		ENTITY_DELETE,
		ENTITY_CREATE
	};


	struct EditorEntityEvent {
		EditorEntityEvent() = default;
		EditorEntityEvent(EditorEntityEventType _event_type, std::vector<uint64_t> _affected_entities) :
			affected_entities(_affected_entities), event_type(_event_type) {}

		std::vector<uint64_t> affected_entities;
		std::vector<std::string> serialized_entities_before;
		std::vector<std::string> serialized_entities_after;
		EditorEntityEventType event_type;
	};

	constexpr unsigned MAX_EVENT_HISTORY = 50;

	class EditorEventStack {
	public:
		EditorEventStack() = default;

		void SetContext(Scene*& p_context, std::vector<uint64_t>* editor_selected_entities) {
			mp_scene_context = &p_context; mp_editor_selected_entities = editor_selected_entities;
		}

		void PushEvent(const EditorEntityEvent& e);
		void Undo();
		void Redo();
		void Clear();

	private:
		std::optional<EditorEntityEvent> GetMostRecentEvent() { return m_events.empty() ? std::nullopt : std::make_optional(m_events[m_events.size() - 1]); }

		int m_active_index = -1;

		std::deque<EditorEntityEvent> m_events;

		Scene** mp_scene_context = nullptr;
		std::vector<uint64_t>* mp_editor_selected_entities;
	};
}
