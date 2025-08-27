cmake_minimum_required(VERSION 3.8)

# This exposes the following targets:
# ORNG_CORE - contains entire ORNG library, this requires ORNG_BUILD_CORE to be ON, and physx to be installed via vcpkg and available to link to (see how physx is used in this file)
# ORNG_RENDERING - contains all OpenGL rendering functionality
# ORNG_SCENE - contains all scene functionality
# ORNG_UTIL - contains utility functionality

# You should link against one or more of these targets, and use target_include_directories(YOUR_TARGET PRIVATE ${ORNG_CORE_INCLUDE_DIRS}) somewhere in your CMakeLists

# Look at ORNG_CORE_BINARIES for the dlls you need to place next to your .exe
# ORNG_RENDERING requires the glew binary
# ORNG_SCENE requires the fmod binary (for audio)

# If you get unresolved symbol errors, check the file the symbol is defined in (should be a matching .cpp file to the .h file), then check which target here compiles it,...
# ... then link against that target too.

project(ORNG_CORE)
set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})

set(ORNG_RENDERING_SOURCES
		src/rendering/MeshAsset.cpp
		src/rendering/MeshInstanceGroup.cpp
		src/rendering/Quad.cpp
		src/rendering/Renderer.cpp
		src/rendering/Textures.cpp
		src/stb_image.cpp
		src/rendering/VAO.cpp
		src/core/GLStateManager.cpp
		src/core/Window.cpp

		src/framebuffers/Framebuffer.cpp
		src/framebuffers/FramebufferLibrary.cpp

		src/shaders/Shader.cpp
		src/shaders/ShaderLibrary.cpp

		src/components/managers/MeshComponentManager.cpp
		src/components/managers/PointlightComponentManager.cpp
		src/components/managers/SpotlightComponentManager.cpp
		src/components/managers/ParticleSystem.cpp
		src/components/managers/SceneUBOSystem.cpp
		src/components/managers/EnvMapSystem.cpp
)

set(ORNG_UTIL_SOURCES
	src/util/ExtraMath.cpp
	src/util/Log.cpp
	src/util/Timers.cpp
	src/util/TimeStep.cpp
	src/util/UUID.cpp
	src/util/util.cpp
	src/misc/Interpolator.cpp
)

set(ORNG_SCENE_SOURCES
	src/components/MeshComponent.cpp
	src/components/TransformComponent.cpp
	src/components/ScriptComponent.cpp
	src/components/Component.cpp
	src/components/lights/LightComponents.cpp
	src/components/Camera.cpp
	src/scene/Scene.cpp
	src/scene/SceneEntity.cpp
	src/scene/SceneSerializer.cpp
	src/components/managers/AudioSystem.cpp
	src/components/TransfomHierarchySystem.cpp
	src/components/ParticleEmitterComponent.cpp
	src/components/managers/ScriptSystem.cpp
	src/components/AudioComponent.cpp
	src/components/PhysicsComponent.cpp
	src/components/managers/PhysicsSystem.cpp
	src/audio/AudioEngine.cpp
)

set(ORNG_LIB_SOURCES
	# These aren't included in the rendering library as they have dependencies on the scene
	src/rendering/renderpasses/SSAOPass.cpp
	src/rendering/renderpasses/BloomPass.cpp
	src/rendering/renderpasses/GBufferPass.cpp
	src/rendering/renderpasses/DepthPass.cpp
	src/rendering/renderpasses/VoxelPass.cpp
	src/rendering/renderpasses/LightingPass.cpp
	src/rendering/renderpasses/FogPass.cpp
	src/rendering/renderpasses/TransparencyPass.cpp
	src/rendering/renderpasses/PostProcessPass.cpp

	src/rendering/EnvMapLoader.cpp

    src/core/Application.cpp
    src/core/Window.cpp
    src/events/EventManager.cpp
    src/events/Events.cpp
    src/layers/LayerStack.cpp
    src/pch/pch.cpp
    src/rendering/SceneRenderer.cpp
    src/scene/GridMesh.cpp

    src/assets/AssetManager.cpp
    extern/implot/implot.cpp
    extern/implot/implot_items.cpp
    src/scripting/ScriptingEngine.cpp
    src/layers/ImGuiLayer.cpp
    src/assets/AssetsImpl.cpp

    src/assets/AssetSerializer.cpp

	src/misc/LoggerUI.cpp
	src/misc/ExtraUI.cpp
)

add_library(ORNG_RENDERING STATIC ${ORNG_RENDERING_SOURCES})
add_library(ORNG_UTIL STATIC ${ORNG_UTIL_SOURCES})
add_library(ORNG_SCENE STATIC ${ORNG_SCENE_SOURCES})

if (ORNG_BUILD_CORE)
	add_library(ORNG_CORE STATIC ${ORNG_LIB_SOURCES})
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(ORNG_RENDERING PRIVATE _DEBUG)
	target_compile_definitions(ORNG_UTIL PRIVATE _DEBUG)
	target_compile_definitions(ORNG_SCENE PRIVATE _DEBUG)
else()
	target_compile_definitions(ORNG_RENDERING PRIVATE NDEBUG)
	target_compile_definitions(ORNG_UTIL PRIVATE NDEBUG)
	target_compile_definitions(ORNG_SCENE PRIVATE NDEBUG)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(ORNG_CORE PRIVATE /W4 /WX)
	target_compile_options(ORNG_RENDERING PRIVATE /W4 /WX)
	target_compile_options(ORNG_UTIL PRIVATE /W4 /WX)
	target_compile_options(ORNG_SCENE PRIVATE /W4 /WX)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(WARNING_FLAGS
		-Werror
		-Wall
		-Wno-c++98-compat
		-Wno-c++98-compat-pedantic
		-Wno-c++98-c++11-compat-pedantic
		-Wno-reserved-identifier
	)

	target_compile_options(ORNG_CORE PRIVATE ${WARNING_FLAGS})
	target_compile_options(ORNG_RENDERING PRIVATE ${WARNING_FLAGS})
	target_compile_options(ORNG_UTIL PRIVATE ${WARNING_FLAGS})
	target_compile_options(ORNG_SCENE PRIVATE ${WARNING_FLAGS})
endif()


set(ASSIMP_BUILD_STATIC_LIB ON)
set(ASSIMP_WARNINGS_AS_ERRORS OFF)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/extern/glfw)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/extern/glew-cmake)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/extern/assimp)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/extern/spdlog)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/extern/imgui)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/extern/yaml)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/extern/VRlib)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/extern/jolt/Build)

option(TRACY_ENABLE "" OFF)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/extern/tracy)

target_compile_options(glfw PRIVATE /MD$<$<CONFIG:Debug>:d>)

target_include_directories(imgui PUBLIC 
extern/imgui
extern/glfw/include
)

find_package(OpenGL REQUIRED)

set(ORNG_CORE_INCLUDE_DIRS
	${CMAKE_CURRENT_LIST_DIR}/extern/spdlog/include
	${CMAKE_CURRENT_LIST_DIR}/extern/assimp/include
	${CMAKE_CURRENT_LIST_DIR}/extern/assimp/build/include
	${CMAKE_CURRENT_LIST_DIR}/extern/glfw/include
	${CMAKE_CURRENT_BINARY_DIR}/extern/assimp/include
	${CMAKE_CURRENT_LIST_DIR}/extern/glew-cmake/include
	${CMAKE_CURRENT_LIST_DIR}/extern
	${CMAKE_CURRENT_LIST_DIR}/extern/imgui
	${CMAKE_CURRENT_LIST_DIR}/headers
	${CMAKE_CURRENT_LIST_DIR}/extern/yaml/include
	${CMAKE_CURRENT_LIST_DIR}/extern/plog/include
	${CMAKE_CURRENT_LIST_DIR}/extern/bitsery/include
	${CMAKE_CURRENT_LIST_DIR}/extern/implot
	${CMAKE_CURRENT_LIST_DIR}/extern/tracy/public/tracy
	${CMAKE_CURRENT_LIST_DIR}/extern/jolt
	${CMAKE_CURRENT_LIST_DIR}/../vcpkg_installed/x64-windows/include
	${CMAKE_CURRENT_LIST_DIR}/../vcpkg_installed/x64-windows/include/physx
	${CMAKE_CURRENT_BINARY_DIR}/../vcpkg_installed/x64-windows/include
	${CMAKE_CURRENT_BINARY_DIR}/../vcpkg_installed/x64-windows/include/physx
	${VR-CORE-INCLUDES}
	CACHE STRING "" FORCE
)


if (ORNG_BUILD_CORE)
	target_include_directories(ORNG_CORE PUBLIC ${ORNG_CORE_INCLUDE_DIRS})
	target_link_libraries(ORNG_CORE PUBLIC
	ORNG_RENDERING
	ORNG_UTIL
	ORNG_SCENE
	OpenGL::GL
	glfw
	libglew_shared
	assimp
	spdlog::spdlog_header_only
	${ASSIMP_LIBRARY}
	Tracy::TracyClient
	VR-CORE
	Jolt
	)
	target_precompile_headers(ORNG_CORE PUBLIC headers/pch/pch.h)
endif()

target_include_directories(ORNG_RENDERING PUBLIC ${ORNG_CORE_INCLUDE_DIRS})
target_include_directories(ORNG_UTIL PUBLIC ${ORNG_CORE_INCLUDE_DIRS})
target_include_directories(ORNG_SCENE PUBLIC ${ORNG_CORE_INCLUDE_DIRS})

target_link_libraries(ORNG_RENDERING PUBLIC
	OpenGL::GL
	glfw
	libglew_shared
	assimp
	${ASSIMP_LIBRARY}
	Tracy::TracyClient
	ORNG_UTIL
)

target_link_libraries(ORNG_UTIL PUBLIC
	spdlog::spdlog_header_only
	Tracy::TracyClient
)

target_link_libraries(ORNG_SCENE PUBLIC
	yaml-cpp
	${YAML_CPP_LIBRARIES}
	ORNG_UTIL
	Jolt
)

# Link FMOD to ORNG_CORE
if (ORNG_BUILD_CORE AND NOT ORNG_DISABLE_FMOD)
	target_link_libraries(ORNG_CORE PUBLIC ${CMAKE_CURRENT_LIST_DIR}/extern/fmod/api/core/lib/x64/fmod$<$<CONFIG:Debug>:L>_vc.lib)
	target_include_directories(ORNG_CORE PUBLIC "${CMAKE_CURRENT_LIST_DIR}/extern/fmod/api/core/inc")
endif()

# Link FMOD to ORNG_SCENE
target_link_libraries(ORNG_SCENE PUBLIC ${CMAKE_CURRENT_LIST_DIR}/extern/fmod/api/core/lib/x64/fmod$<$<CONFIG:Debug>:L>_vc.lib)
target_include_directories(ORNG_SCENE PUBLIC "${CMAKE_CURRENT_LIST_DIR}/extern/fmod/api/core/inc")

target_precompile_headers(ORNG_RENDERING PUBLIC headers/pch/pch.h)
target_precompile_headers(ORNG_UTIL PUBLIC headers/pch/pch.h)

# Required binaries for engine
if (CMAKE_BUILD_TYPE MATCHES Debug)
	set(CMAKE_CONFIGURATION_TYPES "Debug" CACHE STRING "" FORCE)
	set(ORNG_CORE_BINARIES
	${CMAKE_CURRENT_LIST_DIR}/extern/fmod/api/core/lib/x64/fmodL.dll
	${CMAKE_CURRENT_BINARY_DIR}/extern/glew-cmake/bin/glew-sharedd.dll
	${GPU_LIBRARY_LOCATION}
	${GPU_DEVICE_LIBRARY_LOCATION}
	CACHE STRING "Copy these into your .exe directory"
	)
endif()


if(CMAKE_BUILD_TYPE MATCHES Release OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
	set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "" FORCE)
	set(ORNG_CORE_BINARIES
		${CMAKE_CURRENT_LIST_DIR}/extern/fmod/api/core/lib/x64/fmod.dll
		${CMAKE_CURRENT_BINARY_DIR}/extern/glew-cmake/bin/glew-shared.dll
		${GPU_LIBRARY_LOCATION}
		${GPU_DEVICE_LIBRARY_LOCATION}
		CACHE STRING "Copy these into your .exe directory" FORCE
	)
elseif(NOT CMAKE_BUILD_TYPE MATCHES Debug)
    message(FATAL_ERROR "Only debug and release builds are supported")
endif()

if (ORNG_BUILD_CORE)
	target_compile_definitions(ORNG_CORE PUBLIC
	ORNG_CORE_MAIN_DIR="${CMAKE_CURRENT_LIST_DIR}"
	ORNG_CORE_LIB_DIR="${CMAKE_BINARY_DIR}/ORNG-Core/"
	GLM_ENABLE_EXPERIMENTAL
	ORNG_CORE_DEBUG_BINARY_DIR="${CMAKE_BINARY_DIR}/../debug"
	ORNG_CORE_RELEASE_BINARY_DIR="${CMAKE_BINARY_DIR}/../release"
	)
endif()

target_compile_definitions(ORNG_RENDERING PUBLIC
	ORNG_CORE_MAIN_DIR="${CMAKE_CURRENT_LIST_DIR}"
	ORNG_CORE_LIB_DIR="${CMAKE_BINARY_DIR}/ORNG-Core/"
	GLM_ENABLE_EXPERIMENTAL
	ORNG_CORE_DEBUG_BINARY_DIR="${CMAKE_BINARY_DIR}/../debug"
	ORNG_CORE_RELEASE_BINARY_DIR="${CMAKE_BINARY_DIR}/../release"
)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/res/ DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/res/core-res")





