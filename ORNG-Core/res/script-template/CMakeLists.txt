cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /MP /bigobj" CACHE INTERNAL "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /MP /O2" CACHE INTERNAL "" FORCE)

project(SCRIPTS)

set(ENGINE_BASE_DIR REPLACE_ME_ENGINE_BASE_DIR) # changed in engine

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
set(ENGINE_BINARY_DIR REPLACE_ME_ENGINE_DEBUG_BINARY_DIR) # changed in engine
else()
set(ENGINE_BINARY_DIR REPLACE_ME_ENGINE_RELEASE_BINARY_DIR) # changed in engine
endif()

set(SCRIPT_INCLUDE_DIRS
${ENGINE_BASE_DIR}/ORNG-Core/extern/spdlog/include
${ENGINE_BASE_DIR}/ORNG-Core/extern/assimp/include
${ENGINE_BASE_DIR}/ORNG-Core/extern/assimp/build/include
${ENGINE_BASE_DIR}/ORNG-Core/extern/glfw/include
${ENGINE_BASE_DIR}/ORNG-Core/extern/glew-cmake/include
${ENGINE_BASE_DIR}/ORNG-Core/extern
${ENGINE_BASE_DIR}/ORNG-Core/extern/imgui
${ENGINE_BASE_DIR}/ORNG-Core/headers
${ENGINE_BASE_DIR}/ORNG-Core/extern/fastnoise2/include
${ENGINE_BASE_DIR}/ORNG-Core/extern/yaml/include
${ENGINE_BASE_DIR}/ORNG-Core/extern/plog/include
${ENGINE_BASE_DIR}/ORNG-Core/extern/bitsery/include
${ENGINE_BASE_DIR}/ORNG-Core/extern/implot
${ENGINE_BASE_DIR}/extern/vcpkg/packages/physx_x64-windows/include/
${ENGINE_BASE_DIR}/extern/vcpkg/packages/physx_x64-windows/include/physx
"${ENGINE_BASE_DIR}/ORNG-Core/extern/tracy/public/tracy"
)

set(SCRIPT_LIBS 
${ENGINE_BINARY_DIR}/ORNG-Core/ORNG_CORE.lib
${ENGINE_BINARY_DIR}/ORNG-Core/extern/imgui/imgui.lib
)

# DO NOT MODIFY OR DELETE THIS LINE - SCRIPT START

# DO NOT MODIFY OR DELETE THIS LINE - SCRIPT END

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
