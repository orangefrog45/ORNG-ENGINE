cmake_minimum_required(VERSION 3.8)

project(ORNG_CORE)
set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})

set(ORNG_LIB_SOURCES 

src/stb_image.cpp
src/components/Camera.cpp
src/components/Component.cpp
src/components/MeshComponent.cpp
src/components/PhysicsComponent.cpp
src/components/TransformComponent.cpp
src/components/lights/LightComponents.cpp
src/components/managers/MeshComponentManager.cpp
src/components/managers/PhysicsSystem.cpp
src/components/managers/PointlightComponentManager.cpp
src/components/managers/SpotlightComponentManager.cpp
src/core/Application.cpp
src/core/CodedAssets.cpp
src/core/GLStateManager.cpp
src/core/Window.cpp
src/events/EventManager.cpp
src/events/Events.cpp
src/framebuffers/Framebuffer.cpp
src/framebuffers/FramebufferLibrary.cpp
src/layers/LayerStack.cpp
src/pch/pch.cpp
src/physics/physics.cpp
src/rendering/EnvMapLoader.cpp
src/rendering/MeshAsset.cpp
src/rendering/MeshInstanceGroup.cpp
src/rendering/Quad.cpp
src/rendering/Renderer.cpp
src/rendering/SceneRenderer.cpp
src/rendering/Textures.cpp
src/rendering/VAO.cpp
src/scene/GridMesh.cpp
src/scene/Scene.cpp
src/scene/SceneEntity.cpp
src/scene/ScenePostProcessing.cpp
src/scene/SceneSerializer.cpp
src/scene/Skybox.cpp
src/shaders/Shader.cpp
src/shaders/ShaderLibrary.cpp
src/terrain/ChunkLoader.cpp
src/terrain/Terrain.cpp
src/terrain/TerrainChunk.cpp
src/terrain/TerrainGenerator.cpp
src/terrain/TerrainQuadtree.cpp
src/util/ExtraMath.cpp
src/util/Log.cpp
src/util/Timers.cpp
src/util/TimeStep.cpp
src/util/UUID.cpp
src/core/AssetManager.cpp
extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD.cpp
extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD_avx2.cpp
extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD_avx512.cpp
extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD_neon.cpp
extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD_internal.cpp
extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD_sse2.cpp
extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD_sse41.cpp

 "src/scripting/ScriptingEngine.cpp"   "src/audio/AudioEngine.cpp" "src/audio/AudioSystem.cpp")

add_library(ORNG_CORE STATIC
${ORNG_LIB_SOURCES}
)



add_subdirectory(${PROJECT_SOURCE_DIR}/extern/glfw)
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/glew-cmake)
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/assimp)
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/spdlog)
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/imgui)
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/yaml)

target_compile_options(glfw PRIVATE /MD$<$<CONFIG:Debug>:d>)

target_include_directories(imgui PUBLIC 
extern/imgui
extern/glfw/include
)

find_package(OpenGL REQUIRED)


target_include_directories(ORNG_CORE PUBLIC
extern/spdlog/include
extern/assimp/include
extern/assimp/build/include
extern/glfw/include
extern/glew-cmake/include
extern/physx/physx/include
extern
extern/imgui
headers
extern/fastnoise2/include
extern/yaml/include
extern/plog/include
extern/bitsery/include
)

set(TARGET_BUILD_PLATFORM "windows")
set(PHYSX_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/extern/physx/physx)
set(PXSHARED_PATH ${PHYSX_ROOT_DIR}/../pxshared)
set(PXSHARED_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CMAKEMODULES_VERSION "1.27")
set(CMAKEMODULES_PATH ${PHYSX_ROOT_DIR}/../externals/cmakemodules)
set(PX_OUTPUT_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/externals/physx)
set(PX_OUTPUT_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/externals/physx)


set(PX_BUILDSNIPPETS OFF CACHE BOOL "Generate the snippets" FORCE)
set(PX_BUILDPUBLICSAMPLES OFF CACHE BOOL "Generate the samples projects" FORCE)
set(PX_GENERATE_STATIC_LIBRARIES ON CACHE BOOL "Generate static libraries" FORCE)
set(PX_FLOAT_POINT_PRECISE_MATH OFF CACHE BOOL "Float point precise math" FORCE)
set(NV_USE_STATIC_WINCRT OFF CACHE BOOL "Use the statically linked windows CRT" FORCE)
set(NV_USE_DEBUG_WINCRT ON CACHE BOOL "Use the debug version of the CRT" FORCE)

set( PHYSX_LIBRARIES
	PhysXExtensions
	PhysX
	PhysXPvdSDK
	PhysXVehicle
	PhysXCharacterKinematic
	PhysXCooking
	PhysXCommon
	PhysXFoundation
)



add_compile_definitions(PX_PHYSX_STATIC_LIB PX_SUPPORT_GPU_PHYSX)
add_subdirectory(extern/physx/physx/compiler/public)


target_include_directories(ORNG_CORE
	PUBLIC 
	"physx-complete/physx/physx/include/" 
	"physx-complete/physx/pxshared/include/" 
	"physx-complete/physx/physx/snippets/"
	"${CMAKE_CURRENT_LIST_DIR}/extern/fmod/api/core/inc"
)


target_link_libraries(ORNG_CORE PRIVATE 
OpenGL::GL
glfw
libglew_static
assimp
spdlog::spdlog_header_only
yaml-cpp
${CMAKE_CURRENT_LIST_DIR}/extern/fmod/api/core/lib/x64/fmod$<$<CONFIG:Debug>:L>_vc.lib
${YAML_CPP_LIBRARIES}
${ASSIMP_LIBRARY}
${PHYSX_LIBRARIES}
)

target_precompile_headers(ORNG_CORE PUBLIC
headers/pch/pch.h
)


target_compile_definitions(ORNG_CORE PUBLIC ORNG_CORE_MAIN_DIR="${CMAKE_CURRENT_LIST_DIR}")
target_compile_definitions(ORNG_CORE PUBLIC ORNG_CORE_LIB_DIR="${CMAKE_BINARY_DIR}/ORNG-Core/")
target_compile_definitions(ORNG_CORE PUBLIC PHYSX_BIN_OUTPUT_DIR="${PHYSX_ROOT_LIB_DIR}")


if (CMAKE_BUILD_TYPE MATCHES Debug)
set(ORNG_CORE_BINARIES 
${CMAKE_CURRENT_LIST_DIR}/extern/fmod/api/core/lib/x64/fmodL.dll
${CMAKE_CURRENT_LIST_DIR}/../ORNG-Core/dll/debug/PhysXGpu_64.dll
${CMAKE_CURRENT_LIST_DIR}/../ORNG-Core/dll/debug/PhysXDevice64.dll
CACHE string "Copy these into your .exe directory"
)
elseif(CMAKE_BUILD_TYPE MATCHES Release OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
set(ORNG_CORE_BINARIES 
${CMAKE_CURRENT_LIST_DIR}/extern/fmod/api/core/lib/x64/fmod.dll
${CMAKE_CURRENT_LIST_DIR}/../ORNG-Core/dll/release/PhysXGpu_64.dll
${CMAKE_CURRENT_LIST_DIR}/../ORNG-Core/dll/release/PhysXDevice64.dll
CACHE string "Copy these into your .exe directory"
)
else()
    message(FATAL_ERROR "Only debug and release builds are supported")
endif()


file(COPY ${CMAKE_CURRENT_LIST_DIR}/res DESTINATION "${CMAKE_BINARY_DIR}/ORNG-Core/")
file(COPY ${CMAKE_CURRENT_LIST_DIR}/extern/vswhere.exe DESTINATION "${CMAKE_BINARY_DIR}/ORNG-Core/extern/")





