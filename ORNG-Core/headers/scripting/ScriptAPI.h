#pragma once

/* This file sets up all interfaces and includes for external scripts to be connected to the engine
* Include paths are invalid if not used in a project - do not include this file anywhere except in a script
*/

#include <random>
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


