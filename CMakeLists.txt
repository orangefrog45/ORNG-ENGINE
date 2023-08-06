cmake_minimum_required(VERSION 3.3)
set (CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
project(ORNG_CORE)
project(ORNG_EDITOR)
set(CMAKE_COMPILE_WARNING_AS_ERROR OFF)
set(COMPILE_WARNING_AS_ERROR OFF)

# Disable treating warnings as errors for MSVC
if(MSVC)
    add_compile_options(/WX-)
endif()

# Disable treating warnings as errors for GCC and Clang
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wno-error)
endif()

add_subdirectory("ORNG-Core")
add_subdirectory("ORNG Editor")
