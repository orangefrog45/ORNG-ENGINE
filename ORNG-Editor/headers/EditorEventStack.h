#pragma once



namespace ORNG {
	class Scene;

	enum EditorEventType {
		TRANSFORM_UPDATE,
		ENTITY_DELETE,
		ENTITY_CREATE
	};


	struct EditorEvent {
		std::vector<uint64_t> affected_entities;
		std::vector<std::string> serialized_entities_before;
		std::vector<std::string> serialized_entities_after;
		EditorEventType event_type;
	};

	class EditorEventStack {
	public:
		EditorEventStack() = default;
		void SetContext(Scene*& p_context) { mp_scene_context = &p_context; };
		void PushEvent(const EditorEvent& e) {
			if (m_active_index != m_events.size() - 1)
				m_events.erase(m_events.begin() + glm::max(m_active_index, 0), m_events.end());

			m_events.push_back(e);
			m_active_index = m_events.size() - 1;
		};

		// Newest = last in vector
		// Oldest = [0] in vector
		// When new event added, events at index higher than m_active_index get wiped, then event is pushed back
		//
		void Undo();
		void Redo();

	private:

		std::optional<EditorEvent> GetMostRecentEvent() { return m_events.empty() ? std::nullopt : std::make_optional(m_events[m_events.size() - 1]); };

		int m_active_index = -1;
		std::vector<EditorEvent> m_events;

		Scene** mp_scene_context = nullptr;
	};
}