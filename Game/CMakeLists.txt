cmake_minimum_required(VERSION 3.8)

project(FRACTAL_GAME)

add_executable(FRACTAL_GAME 
../ORNG-Editor/src/EditorLayer.cpp
../ORNG-Editor/src/AssetManagerWindow.cpp
src/GameLayer.cpp
src/main.cpp
)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/../ORNG-Editor/res DESTINATION "${CMAKE_BINARY_DIR}/Game/")



target_include_directories(FRACTAL_GAME PUBLIC
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
"../ORNG-Editor"
"../ORNG-Editor/headers"
headers
)

target_link_libraries(FRACTAL_GAME PUBLIC 
ORNG_CORE
imguizmo
imgui
)

target_include_directories(imguizmo PUBLIC 
../ORNG-Editor/extern/imguizmo
../ORNG-Core/extern/imgui
)


target_precompile_headers(FRACTAL_GAME REUSE_FROM ORNG_CORE)


add_custom_command(TARGET FRACTAL_GAME POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:FRACTAL_GAME>)
foreach(core_binary IN LISTS ORNG_CORE_BINARIES)
    add_custom_command(TARGET FRACTAL_GAME POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${core_binary}
        $<TARGET_FILE_DIR:FRACTAL_GAME>)
endforeach()
