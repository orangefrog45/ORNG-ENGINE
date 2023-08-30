#ifndef EVENTS_H
#define EVENTS_H
#include "components/Component.h"



namespace ORNG::Events {
	class SceneEntity;

	struct Event {
		enum EventType {
			INVALID_TYPE = 0,
			ENGINE_RENDER = 1,
			ENGINE_UPDATE = 2,
			WINDOW_RESIZE = 3,
		};


		EventType event_type = EventType::INVALID_TYPE;
	};

	struct EngineCoreEvent : public Event {

	};

	struct WindowEvent : public Event {

		glm::ivec2 old_window_size;
		glm::ivec2 new_window_size;
	};

	struct MouseEvent : public Event { // Not yet implemented

		//Input::MouseBindings mouse_button;
		glm::ivec2 mouse_pos_new;
		glm::ivec2 mouse_pos_old;
	};


	struct KeyEvent : public Event {
		char key;
		uint8_t event_type; // will be InputType enum, can't declare as such due to circular include

	};

	enum class ProjectEventType {
		MATERIAL_DELETED,
		MESH_DELETED,
		MESH_LOADED,
		MATERIAL_LOADED,
		TEXTURE_DELETED,
		TEXTURE_LOADED,
	};

	struct ProjectEvent : public Event {
		ProjectEventType event_type;
		uint8_t* data_payload = nullptr; // Data payload will be a ptr to the asset being modified if it's an asset event
	};

	enum class ECS_EventType {
		COMP_ADDED,
		COMP_UPDATED,
		COMP_DELETED,
		COLLISION,
	};

	template <std::derived_from<Component> T>
	struct ECS_Event : public Event {
		ECS_EventType event_type;
		uint32_t sub_event_type; // E.g a code for "Scaling transform" for a transform component update
		std::vector<T*> affected_components;
	};

	template <std::derived_from<Event> T>
	class EventListener {
	public:
		friend class EventManager;
		EventListener() = default;
		EventListener(const EventListener&) = default;
		~EventListener() { if (OnDestroy) OnDestroy(); }
		std::function<void(const T&)> OnEvent = nullptr;

		// Handle used for deregistration, set by eventmanager on listener registration
		uint32_t GetRegisterID() { return m_entt_handle; }
	private:
		// Given upon listener being registered with EventManager
		uint32_t m_entt_handle = 0;
		// Function given by event manager when listener is registered
		std::function<void()> OnDestroy = nullptr;
	};

	// Shortcut class for listening to single key, not case-sensitive
	// For case sensitivity, use EventListener<KeyEvent> instead (will need to use own logic for key checks)
	class SingleKeyEventListener : public EventListener<KeyEvent> {
		friend class EventManager;
	public:
		SingleKeyEventListener() = delete;
		// Not case sensitive, if listening for 'a' then 'A' will still trigger listener
		explicit SingleKeyEventListener(char key_to_listen) : m_listening_key(std::toupper(key_to_listen)) {};

		// Returns the key being listened for
		char GetListeningKey() { return m_listening_key; }
	private:
		char m_listening_key;
	};

	template<std::derived_from<Component> T>
	class ECS_EventListener : public EventListener<ECS_Event<T>> {
	public:
		uint64_t scene_id = 0;
	};


}

#endif