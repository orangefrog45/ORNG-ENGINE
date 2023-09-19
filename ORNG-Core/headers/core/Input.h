#pragma once
#include "events/EventManager.h"

namespace ScriptInterface {
	class Input;
}

namespace ORNG {
	class Input {
		friend class ScriptInterface::Input;
	public:
		Input() = default;


		// Not case-sensitive
		static bool IsKeyDown(char key) {
			return Get().m_key_states[std::toupper(key)];
		}

		static bool IsMouseDown(MouseButton btn) {
			return Get().m_mouse_states[btn];
		}

		static glm::ivec2 GetMousePos() {
			return Get().m_mouse_position;
		}

		static void Init() {
			Get().I_Init();
		}

		static void SetCursorPos(int x, int y) {
			Events::MouseEvent e_event;
			e_event.event_type = Events::MouseEventType::SET;
			e_event.mouse_action = MOVE;
			e_event.mouse_button = MouseButton::NONE;
			e_event.mouse_pos_new = glm::ivec2(x, y);

			Events::EventManager::DispatchEvent(e_event);
		}

		static Input& Get() {
			static Input s_instance;
			return s_instance;
		}
	private:

		void I_Init() {
			m_key_listener.OnEvent = [this](const Events::KeyEvent& t_event) {
				m_key_states[std::toupper(t_event.key)] = t_event.event_type == InputType::PRESS;
			};

			m_mouse_listener.OnEvent = [this](const Events::MouseEvent& t_event) {
				if (t_event.mouse_action != MOVE && t_event.event_type == Events::MouseEventType::RECEIVE) {
					m_mouse_states[t_event.mouse_button] = static_cast<bool>(t_event.mouse_action);
				}
				m_mouse_position = t_event.mouse_pos_new;
			};
			Events::EventManager::RegisterListener(m_key_listener);
			Events::EventManager::RegisterListener(m_mouse_listener);
		}

		glm::ivec2 m_mouse_position{0, 0};

		std::unordered_map<char, bool> m_key_states;
		std::unordered_map<MouseButton, bool> m_mouse_states;

		Events::EventListener<Events::KeyEvent> m_key_listener;
		Events::EventListener<Events::MouseEvent> m_mouse_listener;
	};
}

