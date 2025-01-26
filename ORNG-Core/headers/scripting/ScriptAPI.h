/* This file sets up all interfaces and includes for external scripts to be connected to the engine
* Include paths are invalid if not used in a project - do not include this file anywhere except in a script
*/
/* TODO: fix logging macros */
#ifdef ORNG_CORE_TRACE
#undef ORNG_CORE_TRACE
#define ORNG_CORE_TRACE(...) ;
#endif
#ifdef ORNG_CORE_INFO
#undef ORNG_CORE_INFO
#define ORNG_CORE_INFO(...) ;
#endif
#ifdef ORNG_CORE_WARN
#undef ORNG_CORE_WARN
#define ORNG_CORE_WARN(...) ;
#endif
#ifdef ORNG_CORE_ERROR
#undef ORNG_CORE_ERROR
#define ORNG_CORE_ERROR(...) ;
#endif
#ifdef ORNG_CORE_CRITICAL
#undef ORNG_CORE_CRITICAL
#define ORNG_CORE_CRITICAL(...) ;
#endif

#include <chrono>
#include <any>
#include <filesystem>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/quaternion.hpp>
#include <fstream>
#include <future>
#include <GL/glew.h>


#include "scene/Scene.h"
#include "scene/SceneEntity.h"
#include "components/ComponentAPI.h"
#include "./uuids.h" // Generated through editor on save


