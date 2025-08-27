cmake_minimum_required(VERSION 3.28)

# If you want to treat this as a dependency and just link against the core ORNG library, look at the ./ORNG-Core/CMakeLists.txt file instead.

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(ORNG_CORE)
project(ORNG_EDITOR)
project(ORNG_RUNTIME)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /MP /bigobj")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /MP /O2")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_RELEASE "")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
#    message(FATAL_ERROR "Compilation only supported with MSVC")
endif()

set(ORNG_BUILD_CORE ON)
add_subdirectory("ORNG-Core")
add_subdirectory("ORNG-Editor")
add_subdirectory("ORNG-Runtime")


