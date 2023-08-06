cmake_minimum_required(VERSION 3.3)

project(ORNG_CORE)

add_library(ORNG_CORE STATIC
src/main.cpp
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
extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD.cpp
extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD_avx2.cpp
extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD_avx512.cpp
extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD_neon.cpp
extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD_internal.cpp
extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD_sse2.cpp
extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD_sse41.cpp


)



add_subdirectory(${PROJECT_SOURCE_DIR}/extern/glfw)
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/glew-cmake)
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/assimp)
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/spdlog)
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/imgui)
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/yaml)

target_compile_options(glfw PRIVATE $<$<CONFIG:Debug>:/MTd>)
target_compile_options(glfw PRIVATE $<$<CONFIG:Release>:/MT>)

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
set(NV_USE_STATIC_WINCRT ON CACHE BOOL "Use the statically linked windows CRT" FORCE)
set(NV_USE_DEBUG_WINCRT ON CACHE BOOL "Use the debug version of the CRT" FORCE)
set(PX_GENERATE_GPU_PROJECT ON CACHE BOOL "Generate GPU Support" FORCE)
set(PX_WINDOWS_KMD ON CACHE BOOL "GPU support for physx" FORCE)


set( PHYSX_LIBRARIES
	PhysXExtensions
	PhysXPvdSDK
	PhysXVehicle
	PhysXCharacterKinematic
	PhysXCharacterKinematic
	PhysXCooking
)

#set(PX_COPY_EXTERNAL_DLL OFF CACHE BOOL "Physx copies external dlls" FORCE)
#set(PUBLIC_RELEASE OFF CACHE BOOL "" FORCE)
add_compile_definitions(PX_PHYSX_STATIC_LIB)
add_subdirectory(extern/physx/physx/compiler/public)


if (TARGET_BUILD_PLATFORM STREQUAL "windows")
    # References NvidiaBuildOptions.cmake to figure out if system is 32/64 bit
    IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
        SET(LIBPATH_SUFFIX "64")
    ELSE()
        SET(LIBPATH_SUFFIX "32")
    ENDIF()
    GetPlatformBinName(PLATFORM_BIN_NAME ${LIBPATH_SUFFIX})
    set(PhysxOutputPath ${PX_OUTPUT_LIB_DIR}/bin/${PLATFORM_BIN_NAME}/)
    message("Physx Output Path: " ${PhysxOutputPath})

    # copy PhysX dll's to build dir. Happens on every build.
    add_custom_command(TARGET ORNG_CORE POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${PhysxOutputPath}" "$<TARGET_FILE_DIR:ORNG_CORE>/.."
    ) 
endif()

MESSAGE("${PHYSX_CXX_FLAGS_DEBUG}")


target_include_directories(ORNG_CORE
	PUBLIC 
	"physx-complete/physx/physx/include/" 
	"physx-complete/physx/pxshared/include/" 
	"physx-complete/physx/physx/snippets/"
)


target_link_libraries(ORNG_CORE PUBLIC 
OpenGL::GL
glfw
libglew_static
assimp
spdlog
yaml-cpp
${YAML_CPP_LIBRARIES}
${ASSIMP_LIBRARY}
${PHYSX_LIBRARIES}
)

target_precompile_headers(ORNG_CORE PUBLIC
headers/pch/pch.h
)
