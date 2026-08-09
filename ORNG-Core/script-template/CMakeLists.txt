cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD REPLACE_ME_CXX_STANDARD)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /MP /bigobj" CACHE INTERNAL "" FORCE)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /MP /O2" CACHE INTERNAL "" FORCE)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}" CACHE INTERNAL "" FORCE)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3" CACHE INTERNAL "" FORCE)
endif()

project(SCRIPTS)
#add_compile_options(/Zi)
set(ENGINE_BASE_DIR REPLACE_ME_ENGINE_BASE_DIR) # changed in engine

set(ENGINE_BINARY_DIR REPLACE_ME_ENGINE_BINARY_DIR) # changed in engine

set(CMAKE_DEBUG_POSTFIX "")

set(SCRIPT_INCLUDE_DIRS
${ENGINE_BASE_DIR}/ORNG-Core/extern/spdlog/include
${ENGINE_BASE_DIR}/ORNG-Core/extern/assimp/include
${ENGINE_BASE_DIR}/ORNG-Core/extern/assimp/build/include
${ENGINE_BASE_DIR}/ORNG-Core/extern/glfw/include
${ENGINE_BASE_DIR}/ORNG-Core/extern/glew-cmake/include
${ENGINE_BASE_DIR}/ORNG-Core/extern
${ENGINE_BASE_DIR}/ORNG-Core/extern/entt/src
${ENGINE_BASE_DIR}/ORNG-Core/extern/imgui
${ENGINE_BASE_DIR}/ORNG-Core/headers
${ENGINE_BASE_DIR}/ORNG-Core/extern/yaml/include
${ENGINE_BASE_DIR}/ORNG-Core/extern/plog/include
${ENGINE_BASE_DIR}/ORNG-Core/extern/bitsery/include
${ENGINE_BASE_DIR}/ORNG-Core/extern/implot
${ENGINE_BASE_DIR}/ORNG-Core/extern/jolt
${ENGINE_BASE_DIR}/ORNG-Core/extern/tracy/public/tracy
${ENGINE_BASE_DIR}/ORNG-Core/extern/VRlib/core/headers
${ENGINE_BASE_DIR}/ORNG-Core/extern/VRlib/extern/OpenXR/include
${ENGINE_BINARY_DIR}/extern/assimp/include
${CMAKE_CURRENT_LIST_DIR}/headers
)

find_package(OpenGL REQUIRED)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")

set(SCRIPT_LIBS
OpenGL::GL
${ENGINE_BINARY_DIR}/ORNG_COREd.lib
${ENGINE_BINARY_DIR}/ORNG_UTILd.lib
${ENGINE_BINARY_DIR}/ORNG_SCENEd.lib
${ENGINE_BINARY_DIR}/ORNG_RENDERINGd.lib
${ENGINE_BINARY_DIR}/extern/VRlib/core/VR-COREd.lib
${ENGINE_BINARY_DIR}/extern/VRlib/extern/OpenXR/src/loader/openxr_loaderd.lib
${ENGINE_BINARY_DIR}/extern/imgui/imguid.lib
${ENGINE_BINARY_DIR}/extern/glfw/src/glfw3.lib
${ENGINE_BINARY_DIR}/extern/glew-cmake/lib/glew-sharedd.lib
${ENGINE_BINARY_DIR}/extern/assimp/lib/assimp-vc143-mtd.lib
${ENGINE_BINARY_DIR}/extern/assimp/contrib/zlib/zlibstaticd.lib
${ENGINE_BINARY_DIR}/extern/yaml/yaml-cppd.lib
${ENGINE_BINARY_DIR}/extern/jolt/Build/Joltd.lib
${ENGINE_BASE_DIR}/ORNG-Core/extern/fmod/api/core/lib/x64/fmodL_vc.lib
${YAML_CPP_LIBRARIES}
)
else()
set(SCRIPT_LIBS
OpenGL::GL
${ENGINE_BINARY_DIR}/ORNG_CORE.lib
${ENGINE_BINARY_DIR}/ORNG_UTIL.lib
${ENGINE_BINARY_DIR}/ORNG_SCENE.lib
${ENGINE_BINARY_DIR}/ORNG_RENDERING.lib
${ENGINE_BINARY_DIR}/extern/VRlib/core/VR-CORE.lib
${ENGINE_BINARY_DIR}/extern/VRlib/extern/OpenXR/src/loader/openxr_loader.lib
${ENGINE_BINARY_DIR}/extern/imgui/imgui.lib
${ENGINE_BINARY_DIR}/extern/glfw/src/glfw3.lib
${ENGINE_BINARY_DIR}/extern/glew-cmake/lib/glew-shared.lib
${ENGINE_BINARY_DIR}/extern/assimp/lib/assimp-vc143-mt.lib
${ENGINE_BINARY_DIR}/extern/assimp/contrib/zlib/zlibstatic.lib
${ENGINE_BINARY_DIR}/extern/yaml/yaml-cpp.lib
${ENGINE_BINARY_DIR}/extern/jolt/Build/Jolt.lib
${ENGINE_BASE_DIR}/ORNG-Core/extern/fmod/api/core/lib/x64/fmod_vc.lib
${YAML_CPP_LIBRARIES}
${PHYSX_LIBS}
)
endif()

# DO NOT MODIFY OR DELETE THIS LINE - SCRIPT START

# DO NOT MODIFY OR DELETE THIS LINE - SCRIPT END

target_precompile_headers(${SCRIPT_TARGETS} PUBLIC ${ENGINE_BASE_DIR}/ORNG-Core/headers/pch/pch.h)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
add_compile_definitions(_DEBUG)
set_target_properties(${SCRIPT_TARGETS} PROPERTIES 
LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin/debug"
ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin/debug"
RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin/debug"
)
else()
add_compile_definitions(NDEBUG)
set_target_properties(${SCRIPT_TARGETS} PROPERTIES 
LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin/release"
ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin/release"
RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin/release"
)
endif()

# DO NOT MODIFY OR DELETE THIS LINE - USER STUFF BELOW
