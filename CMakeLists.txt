cmake_minimum_required(VERSION 3.8)

# If you want to treat this as a dependency and just link against the core ORNG library, look at the ./ORNG-Core/CMakeLists.txt file instead.

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_TOOLCHAIN_FILE "extern/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
set(VCPKG_TARGET_TRIPLET x64-windows)

project(ORNG_CORE)
project(ORNG_EDITOR)
project(ORNG_RUNTIME)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /MP /bigobj" CACHE INTERNAL "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /MP /O2" CACHE INTERNAL "" FORCE)
if(MSVC)
    add_compile_options(/WX- /W2)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "Compilation only supported with MSVC")
endif()

set(ORNG_BUILD_CORE ON)
add_subdirectory("ORNG-Core")
add_subdirectory("ORNG-Editor")
add_subdirectory("ORNG-Runtime")

# EXTERNAL PROJECTS NOT IN ENGINE REPO - COMMENT OUT IF CAUSING ERRORS
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/Game")
#project(FRACTAL_GAME)
#project(HW_MONITOR)
#project(PHYS_GAME)
#add_subdirectory("Game")
#add_subdirectory("HW-Monitor")
#add_subdirectory("PhysicsGame")
endif()


