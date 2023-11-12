/* This file sets up all interfaces and includes for external scripts to be connected to the engine
* Include paths are invalid if not used in a project - do not include this file anywhere except in a script
* All of these includes the script will still compile without (including the core engine headers instead of these copies), however they are needed for correct intellisense
*/
#include <chrono>
#include <any>

// SAFE = fine to use in scripts
// UNSAFE = should only be used in here

// SAFE
#include "./glm/glm.hpp"
#include "./glm/gtx/transform.hpp"
//

// UNSAFE
// Wont be included in intellisense however will still be able to be accessed - if scripts try to access these except in here they will break.
#include "core/FrameTiming.h"
#include "events/EventManager.h"
#include "core/Input.h"
//

// SAFE
#include "Component.h"
#include "Lights.h"
#include "MeshComponent.h"
#include "CameraComponent.h"
#include "ScriptComponent.h"
#include "TransformComponent.h"
#include "PhysicsComponent.h"
#include "AudioComponent.h"
#include "SceneEntity.h"
#include "./SceneScriptInterface.h"
#include "ScriptShared.h"
#include "./uuids.h" // Generated through editor on save
//

#define O_PROPERTY
#define O_CONSTRUCTOR

/* TODO: Provide interface override for logging macros */
#ifdef ORNG_CORE_TRACE
#undef ORNG_CORE_TRACE
#endif
#ifdef ORNG_CORE_INFO
#undef ORNG_CORE_INFO
#endif
#ifdef ORNG_CORE_WARN
#undef ORNG_CORE_WARN
#endif
#ifdef ORNG_CORE_ERROR
#undef ORNG_CORE_ERROR
#endif
#ifdef ORNG_CORE_CRITICAL
#undef ORNG_CORE_CRITICAL
#endif


extern "C" {
	namespace ORNG_Connectors {
		__declspec(dllexport) void SetFrameTimingPtr(ORNG::FrameTiming* p_instance);
	}

	namespace ScriptInterface {
		class Input {
		public:
			static bool IsKeyDown(ORNG::Key key) {
				ORNG::InputType state = mp_input->m_key_states[key];
				return state != ORNG::InputType::RELEASE;
			}

			static bool IsKeyDown(unsigned key) {
				ORNG::InputType state = mp_input->m_key_states[static_cast<ORNG::Key>(std::toupper(key))];
				return state != ORNG::InputType::RELEASE;
			}

			static bool IsKeyPressed(ORNG::Key key) {
				return mp_input->m_key_states[key] == ORNG::InputType::PRESS;
			}

			static bool IsKeyPressed(unsigned key) {
				return mp_input->m_key_states[static_cast<ORNG::Key>(std::toupper(key))] == ORNG::InputType::PRESS;
			}

			static bool IsMouseDown(ORNG::MouseButton btn) {
				return mp_input->m_mouse_states[btn] != ORNG::InputType::RELEASE;
			}

			static bool IsMouseClicked(ORNG::MouseButton btn) {
				return mp_input->m_mouse_states[btn] == ORNG::InputType::PRESS;
			}

			static bool IsMouseDown(unsigned btn) {
				return mp_input->m_mouse_states[static_cast<ORNG::MouseButton>(btn)] != ORNG::InputType::RELEASE;
			}

			static bool IsMouseClicked(unsigned btn) {
				return mp_input->m_mouse_states[static_cast<ORNG::MouseButton>(btn)] == ORNG::InputType::PRESS;
			}

			static glm::ivec2 GetMousePos() {
				return mp_input->m_mouse_position;
			}

			static void SetMousePos(float x, float y) {
				mp_input->SetCursorPos(x, y);
			}

			static void SetInput(ORNG::Input* p_input) {
				mp_input = p_input;
			}
		private:
			inline static ORNG::Input* mp_input = nullptr;
		};

		class FrameTiming {
			friend void ORNG_Connectors::SetFrameTimingPtr(ORNG::FrameTiming* p_instance);
		public:
			static float GetDeltaTime() { return mp_instance->IGetTimeStep(); }
			static float GetElapsedTime() { return mp_instance->total_elapsed_time; }
		private:
			inline static ORNG::FrameTiming* mp_instance = nullptr;
		};
	};



	namespace ORNG_Connectors {
		// Connect main application's input system with dll
		__declspec(dllexport) void SetInputPtr(ORNG::Input* p_input) {
			ScriptInterface::Input::SetInput(p_input);
		}

		// Connect main application's event system with dll
		__declspec(dllexport) void SetEventManagerPtr(ORNG::Events::EventManager* p_instance) {
			ORNG::Events::EventManager::SetInstance(p_instance);
		}

		// Connect main application's frametiming system with dll
		__declspec(dllexport) void SetFrameTimingPtr(ORNG::FrameTiming* p_instance) {
			ScriptInterface::FrameTiming::mp_instance = p_instance;
		}

		__declspec(dllexport) void SetCreateEntityCallback(std::function<ORNG::SceneEntity& (const std::string&)> func) {
			ScriptInterface::Scene::CreateEntity = func;
		}

		__declspec(dllexport) void SetDeleteEntityCallback(std::function<void(ORNG::SceneEntity*)> func) {
			ScriptInterface::Scene::DeleteEntity = func;
		}

		__declspec(dllexport) void SetDuplicateEntityCallback(std::function<ORNG::SceneEntity& (ORNG::SceneEntity&)> func) {
			ScriptInterface::Scene::DuplicateEntity = func;
		}

		__declspec(dllexport) void SetInstantiatePrefabCallback(std::function<ORNG::SceneEntity& (const std::string&)> func) {
			ScriptInterface::Scene::InstantiatePrefab = func;
		}

		__declspec(dllexport) void SetGetEntityCallback(std::function<ORNG::SceneEntity& (uint64_t)> func) {
			ScriptInterface::Scene::GetEntity = func;
		}

		__declspec(dllexport) void SetRaycastCallback(std::function<ORNG::RaycastResults(glm::vec3 origin, glm::vec3 unit_dir, float max_distance)> func) {
			ScriptInterface::Scene::Raycast = func;
		}
	}
}
