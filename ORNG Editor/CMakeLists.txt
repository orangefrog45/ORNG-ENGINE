cmake_minimum_required(VERSION 3.3)

project(ORNG_EDITOR)

add_executable(ORNG_EDITOR 
src/EditorCamera.cpp
src/EditorLayer.cpp
src/main.cpp
src/pch.cpp
)

target_include_directories(ORNG_CORE PUBLIC
"../ORNG Core/extern/spdlog/include"
"../ORNG Core/extern/assimp/include"
"../ORNG Core/extern/assimp/build/include"
"../ORNG Core/extern/glfw/include"
"../ORNG Core/extern/glew-cmake/include"
"../ORNG Core/extern/physx/physx/include"
"../ORNG Core/extern"
"../ORNG Core/extern/imgui"
"../ORNG Core/headers"
"../ORNG Core/extern/fastnoise2/include"
"../ORNG Core/extern/yaml/include"
"../ORNG Core/extern/plog/include"
headers
)

target_link_libraries(ORNG_EDITOR PUBLIC ORNG_CORE)

target_precompile_headers(ORNG_EDITOR REUSE_FROM ORNG_CORE)