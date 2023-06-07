#pragma once
#include "core/Input.h"

namespace ORNG::Events {

	enum class EventType {
		INVALID_TYPE = 0,
		ENGINE_RENDER = 1,
		ENGINE_UPDATE = 2,
		MOUSE_DOWN = 3,
		MOUSE_MOVE = 4,
	};



	struct Event {
		EventType event_type = EventType::INVALID_TYPE;
	};

	struct EngineCoreEvent : public Event {
	};

	struct MouseEvent : public Event {
		Input::MouseBindings mouse_button;
		glm::vec2 mouse_pos_new;
		glm::vec2 mouse_pos_old;
	};



	template <std::derived_from<Event> T>
	struct EventListener {
		std::function<void(const T&)> OnEvent = nullptr;
	};



}