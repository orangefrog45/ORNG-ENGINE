/* This file sets up all interfaces and includes for external scripts to be connected to the engine
* Include paths are invalid if not used in a project - do not include this file anywhere except in a script
* All of these includes the script will still compile without (including the core engine headers instead of these copies), however they are needed for correct intellisense
*/
#define CPP
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
#include "./uuids.h" // Generated through editor on save
//

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
			static bool IsKeyDown(char key) {
				return mp_input->m_key_states[std::toupper(key)];
			}

			static bool IsMouseDown(ORNG::MouseButton btn) {
				return mp_input->m_mouse_states[btn];
			}

			static bool IsMouseDown(int btn) {
				return mp_input->m_mouse_states[static_cast<ORNG::MouseButton>(btn)];
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
			static float GetElapsedTime() { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - mp_instance->m_application_start_time).count() / 1000.0; }
		private:
			inline static ORNG::FrameTiming* mp_instance = nullptr;
		};


		/*constexpr unsigned int CompileTimeRandom() {
			return 1103515245u * static_cast<unsigned int>(__TIME__[7]) +
				12345u * static_cast<unsigned int>(__TIME__[5]) +
				1234567u * static_cast<unsigned int>(__TIME__[3]);
		}

		inline static std::unordered_map<ORNG::SceneEntity*, std::unordered_map<const char*, std::any>> data_bank;

		template<typename T>
		class StateVal {
		public:
			StateVal(const T& init, const char* t_key) : key(t_key) {
				if (data_bank.contains(t_key))
					data = data_bank[t_key];
				else
					data = init;
			}

			~StateVal() {
				data_bank[key] = data;
			}

			T data;
		private:
			const char* key = nullptr;
		};
		*/
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

		__declspec(dllexport) void SetRaycastCallback(std::function<ORNG::RaycastResults(glm::vec3 origin, glm::vec3 unit_dir, float max_distance)> func) {
			ScriptInterface::Scene::Raycast = func;
		}

	}

}
