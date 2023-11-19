#pragma once
#include "events/EventManager.h"

namespace ScriptInterface {
	class Input;
}


namespace ORNG {
	enum Key {
		// Alphabet keys
		A = 65,
		B = 66,
		C = 67,
		D = 68,
		E = 69,
		F = 70,
		G = 71,
		H = 72,
		I = 73,
		J = 74,
		K = 75,
		L = 76,
		M = 77,
		N = 78,
		O = 79,
		P = 80,
		Q = 81,
		R = 82,
		S = 83,
		T = 84,
		U = 85,
		V = 86,
		W = 87,
		X = 88,
		Y = 89,
		Z = 90,

		// Numeric keys
		Zero = 48,
		One = 49,
		Two = 50,
		Three = 51,
		Four = 52,
		Five = 53,
		Six = 54,
		Seven = 55,
		Eight = 56,
		Nine = 57,

		// Function keys
		F1 = 290,
		F2 = 291,
		F3 = 292,
		F4 = 293,
		F5 = 294,
		F6 = 295,
		F7 = 296,
		F8 = 297,
		F9 = 298,
		F10 = 299,
		F11 = 300,
		F12 = 301,

		// Special keys
		Space = 32,
		Enter = 257,
		Tab = 258,
		CapsLock = 280,
		Shift = 340,
		LeftControl = 341,
		RightControl = 345,
		Alt = 342,
		Escape = 256,
		Backspace = 259,
		Delete = 261,
		ArrowUp = 265,
		ArrowDown = 264,
		ArrowLeft = 263,
		ArrowRight = 262,
		PageUp = 266,
		PageDown = 267,
		Home = 268,
		End = 269,
		Insert = 260,
	};


	class Input {
		friend class ScriptInterface::Input;
	public:

		// Not case-sensitive
		// Returns true if key has been pressed this frame or is being held down
		static bool IsKeyDown(Key key) {
			InputType state = Get().m_key_states[key];
			return state != InputType::RELEASE;
		}

		// Not case-sensitive
		// Returns true if key has been pressed this frame or is being held down
		static bool IsKeyDown(unsigned key) {
			InputType state = Get().m_key_states[static_cast<Key>(std::toupper(key))];
			return (state != InputType::RELEASE);
		}

		// Not case-sensitive
		// Returns true only if key has been pressed this frame
		static bool IsKeyPressed(Key key) {
			return Get().m_key_states[key] == InputType::PRESS;
		}

		// Not case-sensitive
		// Returns true only if key has been pressed this frame
		static bool IsKeyPressed(unsigned key) {
			return Get().m_key_states[static_cast<Key>(std::toupper(key))] == InputType::PRESS;
		}


		static bool IsMouseDown(MouseButton btn) {
			return Get().m_mouse_states[btn] != InputType::RELEASE;
		}

		static bool IsMouseClicked(MouseButton btn) {
			return Get().m_mouse_states[btn] == InputType::PRESS;
		}

		static bool IsMouseDown(unsigned btn) {
			return Get().m_mouse_states[static_cast<MouseButton>(btn)] != InputType::RELEASE;
		}

		static bool IsMouseClicked(unsigned btn) {
			return Get().m_mouse_states[static_cast<MouseButton>(btn)] == InputType::PRESS;
		}

		static glm::ivec2 GetMousePos() {
			return Get().m_mouse_position;
		}

		static void Init() {
			Get().I_Init();
		}

		static void OnUpdate() {
			// Press state only valid for one frame, then the key is considered "held"
			for (auto& [key, v] : Get().m_key_states) {
				if (v == InputType::PRESS)
					v = InputType::HELD;
			}

			for (auto& [key, v] : Get().m_mouse_states) {
				if (v == InputType::PRESS)
					v = InputType::HELD;
			}
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
				m_key_states[static_cast<Key>(t_event.key)] = static_cast<InputType>(t_event.event_type);
				};

			m_mouse_listener.OnEvent = [this](const Events::MouseEvent& t_event) {
				auto& state = m_mouse_states[t_event.mouse_button];
				if (t_event.mouse_action != MOVE && t_event.event_type == Events::MouseEventType::RECEIVE) {
					state = static_cast<InputType>(t_event.mouse_action);
				}
				m_mouse_position = t_event.mouse_pos_new;
				};
			Events::EventManager::RegisterListener(m_key_listener);
			Events::EventManager::RegisterListener(m_mouse_listener);
		}

		glm::ivec2 m_mouse_position{ 0, 0 };

		std::unordered_map<Key, InputType> m_key_states;
		std::unordered_map<MouseButton, InputType> m_mouse_states;

		Events::EventListener<Events::KeyEvent> m_key_listener;
		Events::EventListener<Events::MouseEvent> m_mouse_listener;
	};
}
