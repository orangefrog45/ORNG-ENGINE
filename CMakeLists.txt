cmake_minimum_required(VERSION 3.3)
set (CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
project(ORNG_CORE)
project(ORNG_EDITOR)

add_subdirectory("ORNG Core")
add_subdirectory("ORNG Editor")
