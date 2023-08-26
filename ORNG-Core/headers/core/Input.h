#pragma once
#include "events/EventManager.h"

namespace ScriptInterface {
	class Input;
}

namespace ORNG {
	enum class MouseBindings {
		LEFT_BUTTON = 0,
		RIGHT_BUTTON = 1,
		SCROLL = 2,
	};

	class Input {
		friend class ScriptInterface::Input;
	public:
		Input() = default;
		enum InputType {
			PRESS,
			RELEASE
		};


		// Not case-sensitive
		static bool IsKeyDown(char key) {
			return Get().I_IsKeyDown(key);
		}

		static void Init() {
			Get().I_Init();
		}

		static Input& Get() {
			static Input s_instance;
			return s_instance;
		}
	private:

		bool I_IsKeyDown(char key) {
			return m_key_states[std::toupper(key)];
		}
		void I_Init() {
			m_key_listener.OnEvent = [this](const Events::KeyEvent& t_event) {
				m_key_states[std::toupper(t_event.key)] = t_event.event_type == Input::InputType::PRESS;
			};
			Events::EventManager::RegisterListener(m_key_listener);
		}

		std::map<char, bool> m_key_states;
		std::map<int, bool> m_mouse_states;

		Events::EventListener<Events::KeyEvent> m_key_listener;
	};
}

