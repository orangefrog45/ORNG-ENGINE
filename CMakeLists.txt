cmake_minimum_required(VERSION 3.8)
set (CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
project(ORNG_CORE)
project(ORNG_EDITOR)

if(MSVC)
    add_compile_options(/WX- /W4)
      set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Disable treating warnings as errors for GCC and Clang
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wno-error)
endif()

add_subdirectory("ORNG-Core")
add_subdirectory("ORNG Editor")
