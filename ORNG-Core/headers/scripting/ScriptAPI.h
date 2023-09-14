/* This file sets up all interfaces and includes for external scripts to be connected to the engine
* Include paths are invalid if not used in a project - do not include this file anywhere
* All of these includes the script will still compile without, however they are needed for correct intellisense
*/
#include <filesystem>
#include <chrono>
#include <string>
#include <functional>

#include "glm/glm.hpp"
#include "Input.h"

#include "core/FrameTiming.h"
#include "Component.h"
#include "Lights.h"
#include "MeshComponent.h"
#include "CameraComponent.h"
#include "ScriptComponent.h"
#include "TransformComponent.h"
#include "PhysicsComponent.h"

#include "SceneEntity.h"
#include "./SceneScriptInterface.h"
#include "./uuids.h" // Generated through editor on save

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
				return mp_input->I_IsKeyDown(key);
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
			static float GetElapsedTime() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - mp_instance->m_application_start_time).count() / 1000.0; }
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


	}

}
