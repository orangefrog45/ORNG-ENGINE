/* This file sets up all interfaces and includes for external scripts to be connected to the engine
* Include paths are invalid if not used in a project - do not include this file anywhere
* All of these includes the script will still compile without, however they are needed for correct intellisense
*/
#include <filesystem>

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
#include "Scene.h"



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
	}

}
