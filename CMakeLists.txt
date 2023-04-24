cmake_minimum_required(VERSION 3.8)

set (CMAKE_CXX_STANDARD 20)
project(OAR)
option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)

add_executable(OAR
    OAR/src/main.cpp
    OAR/src/Application.cpp
    OAR/src/ExtraMath.cpp
    OAR/src/GLErrorHandling.cpp
    OAR/src/Input.cpp
    OAR/src/Skybox.cpp
    OAR/src/stb_image.cpp
    OAR/src/TimeStep.cpp
    OAR/src/WorldTransform.cpp
    OAR/src/components/Camera.cpp
    OAR/src/components/Component.cpp
    OAR/src/components/GridMesh.cpp
    OAR/src/components/MeshComponent.cpp
    OAR/src/components/SceneEntity.cpp
    OAR/src/components/lights/DirectionalLight.cpp
    OAR/src/components/lights/SpotLightComponent.cpp
    OAR/src/framebuffers/Framebuffer.cpp
    OAR/src/framebuffers/FramebufferLibrary.cpp
    OAR/src/layers/EditorLayer.cpp
    OAR/src/rendering/MeshAsset.cpp
    OAR/src/rendering/MeshInstanceGroup.cpp
    OAR/src/rendering/Quad.cpp
    OAR/src/rendering/Renderer.cpp
    OAR/src/rendering/Scene.cpp
    OAR/src/rendering/Textures.cpp
    OAR/src/rendering/RenderPasses.cpp
    OAR/src/shaders/LightingShader.cpp
    OAR/src/shaders/Shader.cpp
    OAR/src/shaders/ShaderLibrary.cpp
    OAR/src/terrain/ChunkLoader.cpp
    OAR/src/terrain/Terrain.cpp
    OAR/src/terrain/TerrainChunk.cpp
    OAR/src/terrain/TerrainGenerator.cpp
    OAR/src/terrain/TerrainQuadtree.cpp
    OAR/src/util/Log.cpp
    OAR/src/util/ImGuiLib.cpp
    OAR/extern/imgui/imgui.cpp
    OAR/extern/imgui/imgui_demo.cpp
    OAR/extern/imgui/imgui_draw.cpp
    OAR/extern/imgui/imgui_impl_glfw.cpp
    OAR/extern/imgui/imgui_impl_opengl3.cpp
    OAR/extern/imgui/imgui_stdlib.cpp
    OAR/extern/imgui/imgui_tables.cpp
    OAR/extern/imgui/imgui_widgets.cpp

)

add_subdirectory(${PROJECT_SOURCE_DIR}/OAR/extern/glew-cmake)
add_subdirectory(${PROJECT_SOURCE_DIR}/OAR/extern/assimp)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(${PROJECT_SOURCE_DIR}/OAR/extern/glfw)


find_package(OpenGL REQUIRED)

file(COPY OAR/res DESTINATION ./)

target_include_directories(OAR PUBLIC 
    OAR/headers
    OAR/extern/spdlog/include
    OAR/extern/fastnoiselite
    OAR/extern
    OAR/extern/glew-cmake/include
    OAR/extern/glfw/include
    OAR/extern/imgui
    OAR/extern/assimp
    OAR/extern/stb
    OAR/extern/glm
    ${OPENGL_INCLUDE_DIRS}
)

SET (LIBS  libglew_static
     glfw
     assimp
     OpenGL::GL
   )
target_link_libraries(OAR PUBLIC
    ${LIBS}
)

if(WIN32)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${PROJECT_SOURCE_DIR}/OAR/DLL"
		$<TARGET_FILE_DIR:x64)
endif()