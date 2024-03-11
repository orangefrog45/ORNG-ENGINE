#include "core/FrameTiming.h"
#include "events/EventManager.h"
#include "core/Input.h"
#include "ScriptAPI.h"

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

	__declspec(dllexport) void SetCreateEntityCallback(std::function<ORNG::SceneEntity& (const std::string&)> func) {
		ScriptInterface::World::CreateEntity = func;
	}

	__declspec(dllexport) void SetDeleteEntityCallback(std::function<void(ORNG::SceneEntity*)> func) {
		ScriptInterface::World::DeleteEntity = func;
	}

	__declspec(dllexport) void SetDuplicateEntityCallback(std::function<ORNG::SceneEntity& (ORNG::SceneEntity&)> func) {
		ScriptInterface::World::DuplicateEntity = func;
	}

	__declspec(dllexport) void SetInstantiatePrefabCallback(std::function<ORNG::SceneEntity& (const std::string&)> func) {
		ScriptInterface::World::InstantiatePrefab = func;
	}

	__declspec(dllexport) void SetGetEntityByNameCallback(std::function<ORNG::SceneEntity& (const std::string&)> func) {
		ScriptInterface::World::GetEntityByName = func;
	}

	__declspec(dllexport) void SetGetEntityByEnttHandleCallback(std::function<ORNG::SceneEntity& (entt::entity)> func) {
		ScriptInterface::World::GetEntityByEnttHandle = func;
	}


	__declspec(dllexport) void SetGetEntityCallback(std::function<ORNG::SceneEntity& (uint64_t)> func) {
		ScriptInterface::World::GetEntity = func;
	}
	__declspec(dllexport) void SetRaycastCallback(std::function<ORNG::RaycastResults(glm::vec3 origin, glm::vec3 unit_dir, float max_distance)> func) {
		ScriptInterface::World::Raycast = func;
	}

	__declspec(dllexport) void SetOverlapQueryCallback(std::function <ORNG::OverlapQueryResults(physx::PxGeometry&, glm::vec3)> func) {
		ScriptInterface::World::OverlapQuery = func;
	}




}

namespace ScriptInterface {


	glm::vec2 Input::GetMouseDelta() {
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

	void Input::SetMousePos(float x, float y) {
		static_cast<ORNG::Input*>(mp_input)->SetCursorPos(x, y);
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