#pragma once

/* This file sets up all interfaces and includes for external scripts to be connected to the engine
* Include paths are invalid if not used in a project - do not include this file anywhere except in a script
* All of these includes the script will still compile without (including the core engine headers instead of these copies), however they are needed for correct intellisense
*/


#include <any>
#include <filesystem>
#include <chrono>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm/glm.hpp"
#include "glm/glm/gtx/transform.hpp"
#include "glm/glm/gtx/quaternion.hpp"

#include "util/util.h"
#include "glew-cmake/include/GL/glew.h"
#include "components/ComponentAPI.h"
#include "scene/SceneEntity.h"
#include "ScriptShared.h"
#include "../includes/uuids.h" // Generated through editor on save

#define O_CONSTRUCTOR
#define O_PROPERTY
#define O_LINK(x) ;

/* TODO: Provide interface override for logging macros */
#ifdef ORNG_CORE_TRACE
#undef ORNG_CORE_TRACE
#define ORNG_CORE_TRACE(...)
#endif
#ifdef ORNG_CORE_INFO
#undef ORNG_CORE_INFO
#define ORNG_CORE_INFO(...)
#endif
#ifdef ORNG_CORE_WARN
#undef ORNG_CORE_WARN
#define ORNG_CORE_WARN(...)
#endif
#ifdef ORNG_CORE_ERROR
#undef ORNG_CORE_ERROR
#define ORNG_CORE_ERROR(...)
#endif
#ifdef ORNG_CORE_CRITICAL
#undef ORNG_CORE_CRITICAL
#define ORNG_CORE_CRITICAL(...)
#endif



extern "C" {

	namespace ORNG {
		enum Key;
		enum MouseButton;
	}


	namespace ScriptInterface {
		class Input {
		public:

			static glm::vec2 GetMouseDelta();

			static bool IsKeyDown(ORNG::Key key);

			static bool IsKeyDown(unsigned key);

			static bool IsKeyPressed(ORNG::Key key);

			static bool IsKeyPressed(unsigned key);

			static bool IsMouseDown(ORNG::MouseButton btn);

			static bool IsMouseClicked(ORNG::MouseButton btn);

			static bool IsMouseDown(unsigned btn);

			static bool IsMouseClicked(unsigned btn);

			static glm::ivec2 GetMousePos();

			static void SetMousePos(float x, float y);

			static void SetInput(void* p_input);

			static void SetMouseVisible(bool visible);
		private:
			inline static void* mp_input = nullptr;
		};

		class FrameTiming {
		public:
			static float GetDeltaTime();
			static float GetElapsedTime();
			static void SetInstance(void* p_instance) {
				mp_instance = p_instance;
			}
		private:
			inline static void* mp_instance = nullptr;
		};
	};



}
