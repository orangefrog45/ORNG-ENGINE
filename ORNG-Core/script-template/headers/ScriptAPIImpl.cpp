#include "../includes/ScriptAPI.h"
#include "core/FrameTiming.h"
#include "events/EventManager.h"
#include "core/Input.h"
#include "imgui/imgui.h"

namespace physx {
	class PxGeometry;
}

extern "C" {
	// Connect main application's event system with dll
	__declspec(dllexport) void SetEventManagerPtr(ORNG::Events::EventManager* p_instance) {
		ORNG::Events::EventManager::SetInstance(p_instance);
	}

	// Connect main application's input system with dll
	__declspec(dllexport) void SetInputPtr(void* p_input) {
		ScriptInterface::Input::SetInput(p_input);
	}

	// Connect main application's frametiming system with dll
	__declspec(dllexport) void SetFrameTimingPtr(void* p_instance) {
		ScriptInterface::FrameTiming::SetInstance(p_instance);
	}

	// Connect main application's ImGui context with dll
	__declspec(dllexport) void SetImGuiContext(void* p_instance, void* mem_alloc_func, void* free_func) {
		ImGui::SetCurrentContext(static_cast<ImGuiContext*>(p_instance));
		ImGui::SetAllocatorFunctions(static_cast<ImGuiMemAllocFunc>(mem_alloc_func), static_cast<ImGuiMemFreeFunc>(free_func));
	}
}

namespace ScriptInterface {
	glm::ivec2 Input::GetMouseDelta() {
		return static_cast<ORNG::Input*>(mp_input)->m_mouse_position - static_cast<ORNG::Input*>(mp_input)->m_last_mouse_position;
	}

	bool Input::IsKeyDown(ORNG::Key key) {
		ORNG::InputType state = static_cast<ORNG::Input*>(mp_input)->m_key_states[key];
		return state != ORNG::InputType::RELEASE;
	}

	bool Input::IsKeyDown(unsigned key) {
		ORNG::InputType state = static_cast<ORNG::Input*>(mp_input)->m_key_states[static_cast<ORNG::Key>(std::toupper(key))];
		return state != ORNG::InputType::RELEASE;
	}

	bool Input::IsKeyPressed(ORNG::Key key) {
		return static_cast<ORNG::Input*>(mp_input)->m_key_states[key] == ORNG::InputType::PRESS;
	}

	bool Input::IsKeyPressed(unsigned key) {
		return static_cast<ORNG::Input*>(mp_input)->m_key_states[static_cast<ORNG::Key>(std::toupper(key))] == ORNG::InputType::PRESS;
	}

	bool Input::IsMouseDown(ORNG::MouseButton btn) {
		return static_cast<ORNG::Input*>(mp_input)->m_mouse_states[btn] != ORNG::InputType::RELEASE;
	}

	bool Input::IsMouseClicked(ORNG::MouseButton btn) {
		return static_cast<ORNG::Input*>(mp_input)->m_mouse_states[btn] == ORNG::InputType::PRESS;
	}

	bool Input::IsMouseDown(unsigned btn) {
		return static_cast<ORNG::Input*>(mp_input)->m_mouse_states[static_cast<ORNG::MouseButton>(btn)] != ORNG::InputType::RELEASE;
	}

	bool Input::IsMouseClicked(unsigned btn) {
		return static_cast<ORNG::Input*>(mp_input)->m_mouse_states[static_cast<ORNG::MouseButton>(btn)] == ORNG::InputType::PRESS;
	}

	glm::ivec2 Input::GetMousePos() {
		return static_cast<ORNG::Input*>(mp_input)->m_mouse_position;
	}

	void Input::SetMousePos(glm::ivec2 pos) {
		static_cast<ORNG::Input*>(mp_input)->SetCursorPos(pos.x, pos.y);
	}

	void Input::SetInput(void* p_input) {
		mp_input = p_input;
	}

	void Input::SetMouseVisible(bool visible) {
		static_cast<ORNG::Input*>(mp_input)->SetCursorVisible(visible);
	}

	float FrameTiming::GetDeltaTime() { return static_cast<ORNG::FrameTiming*>(mp_instance)->IGetTimeStep(); }

	float FrameTiming::GetElapsedTime() { return static_cast<ORNG::FrameTiming*>(mp_instance)->total_elapsed_time; }
}