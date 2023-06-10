#pragma once
#include "core/Input.h"

namespace ORNG::Events {


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
		glm::vec2 old_window_size;
		glm::vec2 new_window_size;
	};

	struct MouseEvent : public Event {
		Input::MouseBindings mouse_button;
		glm::vec2 mouse_pos_new;
		glm::vec2 mouse_pos_old;
	};



	template <std::derived_from<Event> T>
	struct EventListener {
		friend class EventManager;
		~EventListener() { if (OnDestroy) OnDestroy(); }
		std::function<void(const T&)> OnEvent = nullptr;
	private:
		// Function given by event manager when listener is registered
		std::function<void()> OnDestroy = nullptr;
	};



}