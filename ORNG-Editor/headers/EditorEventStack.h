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
		EditorEntityEvent(EditorEntityEventType _event_type, std::vector<uint64_t> _affected_entities) : event_type(_event_type), affected_entities(_affected_entities) {};
		std::vector<uint64_t> affected_entities;
		std::vector<std::string> serialized_entities_before;
		std::vector<std::string> serialized_entities_after;
		EditorEntityEventType event_type;
	};

	constexpr unsigned MAX_EVENT_HISTORY = 50;

	class EditorEventStack {
	public:
		EditorEventStack() = default;
		void SetContext(Scene*& p_context) { mp_scene_context = &p_context; };
		void PushEvent(const EditorEntityEvent& e) {

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

		void Undo();
		void Redo();

	private:

		std::optional<EditorEntityEvent> GetMostRecentEvent() { return m_events.empty() ? std::nullopt : std::make_optional(m_events[m_events.size() - 1]); };

		int m_active_index = -1;
		std::deque<EditorEntityEvent> m_events;

		Scene** mp_scene_context = nullptr;
	};
}