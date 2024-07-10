cmake_minimum_required(VERSION 3.8)

project(ORNG_EDITOR)

add_executable(ORNG_EDITOR 
src/EditorLayer.cpp
src/AssetManagerWindow.cpp
src/main.cpp
 "headers/Settings.h" 
 "headers/EditorEventStack.h" 
 "src/EditorEventStack.cpp" 

  "headers/LuaCLI.h" "src/LuaCLI.cpp")


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
"../ORNG-Core/extern/bitsery/include"
headers
extern
extern/sol2/include
extern/sol2/include/sol
extern/lua/src



)

target_include_directories(ORNG_CORE PUBLIC headers)

file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
set_property(TARGET ORNG_EDITOR PROPERTY SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src")
target_compile_definitions(ORNG_EDITOR PUBLIC ORNG_EDITOR_LAYER=true)
target_compile_definitions(ORNG_CORE PUBLIC ORNG_EDITOR_LAYER=true)



add_subdirectory(extern/imguizmo)
add_subdirectory(extern/lua)
target_include_directories(imguizmo PUBLIC 
extern/imguizmo
../ORNG-Core/extern/imgui
)


target_link_libraries(ORNG_EDITOR PUBLIC 
ORNG_CORE
imguizmo
imgui
lua::lua
)

target_precompile_headers(ORNG_EDITOR REUSE_FROM ORNG_CORE)
#Get res folder with shaders etc
file(COPY ${CMAKE_CURRENT_LIST_DIR}/res DESTINATION "${CMAKE_BINARY_DIR}/ORNG-Editor/")
file(COPY ${CMAKE_CURRENT_BINARY_DIR}/../ORNG-Core/res DESTINATION "${CMAKE_BINARY_DIR}/ORNG-Editor/")




add_custom_command(TARGET ORNG_EDITOR POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:ORNG_EDITOR>)
foreach(core_binary IN LISTS ORNG_CORE_BINARIES)
    add_custom_command(TARGET ORNG_EDITOR POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${core_binary}
        $<TARGET_FILE_DIR:ORNG_EDITOR>)
endforeach()


