cmake_minimum_required(VERSION 3.8)
set (CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(VCPKG_TARGET_TRIPLET x64-windows)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/extern/vcpkg/scripts/buildsystems/vcpkg.cmake")

project(ORNG_CORE)
project(ORNG_EDITOR)
project(FRACTAL_GAME)
project(ORNG_RUNTIME)



set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /MP /j12 /bigobj" CACHE INTERNAL "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /MP /O2 /j12" CACHE INTERNAL "" FORCE)
if(MSVC)
    add_compile_options(/WX- /W2)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "Compilation only supported with MSVC")
endif()


add_subdirectory("ORNG-Core")
add_subdirectory("ORNG-Editor")
add_subdirectory("Game")
add_subdirectory("ORNG-Runtime")
add_subdirectory("NetGame")



