cmake_minimum_required(VERSION 3.8)

project(ORNG_EDITOR)

add_executable(ORNG_EDITOR 
src/EditorLayer.cpp
src/main.cpp
)


target_include_directories(ORNG_EDITOR PUBLIC
"../ORNG-Core/extern/spdlog/include"
"../ORNG-Core/extern/assimp/include"
"../ORNG-Core/extern/assimp/build/include"
"../ORNG-Core/extern/glfw/include"
"../ORNG-Core/extern/glew-cmake/include"
"../ORNG-Core/extern/physx/physx/include"
"../ORNG-Core/extern"
"../ORNG-Core/extern/imgui"
"../ORNG-Core/headers"
"../ORNG-Core/extern/fastnoise2/include"
"../ORNG-Core/extern/yaml/include"
"../ORNG-Core/extern/plog/include"
headers
)

file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
set_property(TARGET ORNG_EDITOR PROPERTY SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src")


add_subdirectory(extern/imguizmo)
target_include_directories(imguizmo PUBLIC 
extern/imguizmo
../ORNG-Core/extern/imgui
)


target_link_libraries(ORNG_EDITOR PUBLIC 
ORNG_CORE
imguizmo
imgui
)

target_precompile_headers(ORNG_EDITOR REUSE_FROM ORNG_CORE)
#Get res folder with shaders etc
file(COPY ${CMAKE_CURRENT_LIST_DIR}/res DESTINATION "${CMAKE_BINARY_DIR}/ORNG-Editor/")


file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/projects)


# Copying DLL's here, can't figure out how to get physx to do this properly so janky fix for now.
if(CMAKE_BUILD_TYPE MATCHES Debug)
    file(COPY ${CMAKE_CURRENT_LIST_DIR}/../ORNG-Core/dll/debug/PhysXDevice64.dll DESTINATION "${CMAKE_BINARY_DIR}/ORNG-Editor/")
    file(COPY ${CMAKE_CURRENT_LIST_DIR}/../ORNG-Core/dll/debug/PhysXGpu_64.dll DESTINATION "${CMAKE_BINARY_DIR}/ORNG-Editor/")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    file(COPY ${CMAKE_CURRENT_LIST_DIR}/../ORNG-Core/dll/release/PhysXDevice64.dll DESTINATION "${CMAKE_BINARY_DIR}/ORNG-Editor/")
    file(COPY ${CMAKE_CURRENT_LIST_DIR}/../ORNG-Core/dll/release/PhysXGpu_64.dll DESTINATION "${CMAKE_BINARY_DIR}/ORNG-Editor/")
else()
    message(FATAL_ERROR "Only debug and release builds are supported")
endif()

