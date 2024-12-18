cmake_minimum_required(VERSION 3.8)

project(ORNG_EDITOR)

set(ORNG_EDITOR_SRC_FILES 
${CMAKE_CURRENT_LIST_DIR}/src/EditorLayer.cpp
${CMAKE_CURRENT_LIST_DIR}/src/AssetManagerWindow.cpp
${CMAKE_CURRENT_LIST_DIR}/headers/Settings.h
${CMAKE_CURRENT_LIST_DIR}/headers/EditorEventStack.h
${CMAKE_CURRENT_LIST_DIR}/src/EditorEventStack.cpp
${CMAKE_CURRENT_LIST_DIR}/headers/LuaCLI.h
${CMAKE_CURRENT_LIST_DIR}/src/LuaCLI.cpp
CACHE STRING "" FORCE
)

add_executable(ORNG_EDITOR 
src/main.cpp
${ORNG_EDITOR_SRC_FILES}
)


set(ORNG_EDITOR_INCLUDE_DIRS 
${CMAKE_CURRENT_LIST_DIR}/headers
${CMAKE_CURRENT_LIST_DIR}/extern
${CMAKE_CURRENT_LIST_DIR}/extern/sol2/include
${CMAKE_CURRENT_LIST_DIR}/extern/sol2/include/sol
${CMAKE_CURRENT_LIST_DIR}/extern/lua/src
${CMAKE_CURRENT_LIST_DIR}/extern/imguizmo CACHE STRING "" FORCE
)

target_include_directories(ORNG_EDITOR PUBLIC
${ORNG_CORE_INCLUDE_DIRS}
${ORNG_EDITOR_INCLUDE_DIRS}
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

set(ORNG_EDITOR_LIBS
imguizmo
imgui
lua::lua CACHE STRING "" FORCE
)

target_link_libraries(ORNG_EDITOR PUBLIC 
ORNG_CORE
${ORNG_EDITOR_LIBS}
)

target_precompile_headers(ORNG_EDITOR REUSE_FROM ORNG_CORE)
#Get res folder with shaders etc
file(COPY ${CMAKE_CURRENT_LIST_DIR}/res DESTINATION "${CMAKE_BINARY_DIR}/ORNG-Editor/")
file(COPY ${CMAKE_CURRENT_BINARY_DIR}/../ORNG-Core/res DESTINATION "${CMAKE_BINARY_DIR}/ORNG-Editor/")

add_custom_command(TARGET ORNG_EDITOR POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:ORNG_EDITOR>)
foreach(core_binary IN LISTS ORNG_CORE_BINARIES)
    add_custom_command(TARGET ORNG_EDITOR POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${core_binary}
        $<TARGET_FILE_DIR:ORNG_EDITOR>)
endforeach()


