cmake_minimum_required(VERSION 3.8)

project(ORNG_CORE)
set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})

set(ORNG_RENDERING_SOURCES
		src/rendering/EnvMapLoader.cpp
		src/rendering/MeshAsset.cpp
		src/rendering/MeshInstanceGroup.cpp
		src/rendering/Quad.cpp
		src/rendering/Renderer.cpp
		src/rendering/Textures.cpp
		src/rendering/VAO.cpp
		src/rendering/renderpasses/SSAOPass.cpp
		src/rendering/renderpasses/BloomPass.cpp
		src/rendering/renderpasses/GBufferPass.cpp
		src/rendering/renderpasses/DepthPass.cpp
		src/rendering/renderpasses/VoxelPass.cpp
		src/rendering/renderpasses/LightingPass.cpp
		src/rendering/renderpasses/FogPass.cpp
		src/rendering/renderpasses/TransparencyPass.cpp
		src/rendering/renderpasses/PostProcessPass.cpp

		src/framebuffers/Framebuffer.cpp
		src/framebuffers/FramebufferLibrary.cpp

		src/shaders/Shader.cpp
		src/shaders/ShaderLibrary.cpp
)

set(ORNG_UTIL_SOURCES
	src/util/ExtraMath.cpp
	src/util/Log.cpp
	src/util/Timers.cpp
	src/util/TimeStep.cpp
	src/util/UUID.cpp
	src/util/util.cpp
)

set(ORNG_LIB_SOURCES
    src/stb_image.cpp
    src/components/Camera.cpp
    src/components/Component.cpp
    src/components/MeshComponent.cpp
    src/components/PhysicsComponent.cpp
    src/components/TransformComponent.cpp
    src/components/ScriptComponent.cpp
    src/components/lights/LightComponents.cpp
    src/components/managers/MeshComponentManager.cpp
    src/components/managers/PhysicsSystem.cpp
    src/components/managers/PointlightComponentManager.cpp
    src/components/managers/SpotlightComponentManager.cpp
    src/core/Application.cpp
    src/core/GLStateManager.cpp
    src/core/Window.cpp
    src/events/EventManager.cpp
    src/events/Events.cpp
    src/layers/LayerStack.cpp
    src/pch/pch.cpp
    src/physics/physics.cpp
    src/rendering/SceneRenderer.cpp
    src/scene/GridMesh.cpp
    src/scene/Scene.cpp
    src/scene/SceneEntity.cpp
    src/scene/SceneSerializer.cpp
    src/scene/Skybox.cpp
    src/assets/AssetManager.cpp
    extern/implot/implot.cpp
    extern/implot/implot_items.cpp
    src/scripting/ScriptingEngine.cpp
    src/audio/AudioEngine.cpp
    src/components/managers/AudioSystem.cpp
    src/components/TransfomHierarchySystem.cpp
    src/scene/SceneManager.cpp
    src/components/AudioComponent.cpp
    src/layers/ImGuiLayer.cpp
    src/physics/vehicles/BaseVehicle.cpp
    src/physics/vehicles/PhysXVehicleActor.cpp
    src/physics/vehicles/DirectDrive.cpp
    src/components/VehicleComponent.cpp
    src/assets/AssetsImpl.cpp
    src/components/managers/ParticleSystem.cpp
    src/components/ParticleEmitterComponent.cpp
    src/components/managers/ScriptSystem.cpp
    src/assets/AssetSerializer.cpp
    src/components/managers/SceneUBOSystem.cpp
	headers/components/DecalComponent.h


	src/util/LoggerUI.cpp
	src/util/Interpolator.cpp
	src/util/ExtraUI.cpp
)


add_library(ORNG_CORE STATIC ${ORNG_LIB_SOURCES})
add_library(ORNG_RENDERING STATIC ${ORNG_RENDERING_SOURCES})
add_library(ORNG_UTIL STATIC ${ORNG_UTIL_SOURCES})

find_package(unofficial-omniverse-physx-sdk CONFIG REQUIRED)
target_link_libraries(ORNG_CORE PRIVATE unofficial::omniverse-physx-sdk::sdk)

# Optional: import the defined target to copy over the GPU acceleration libraries (3rd party provided by NVIDIA)
if(TARGET unofficial::omniverse-physx-sdk::gpu-library)
	add_custom_command(TARGET ORNG_CORE POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE:unofficial::omniverse-physx-sdk::gpu-device-library>
			$<TARGET_FILE_DIR:ORNG_CORE>)
	target_compile_definitions(ORNG_CORE PUBLIC PHYSX_GPU_ACCELERATION_AVAILABLE)
else()
	message(WARNING "GPU acceleration library target not defined - GPU acceleration will NOT be available!")
endif()


set(ASSIMP_BUILD_STATIC_LIB ON)
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/glfw)
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/glew-cmake)
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/assimp)
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/spdlog)
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/imgui)
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/yaml)

option(TRACY_ENABLE "" OFF)
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/tracy)

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
${CMAKE_CURRENT_BINARY_DIR}/../vcpkg_installed/x64-windows/include
${CMAKE_CURRENT_BINARY_DIR}/../vcpkg_installed/x64-windows/include/physx
CACHE STRING "" FORCE
)

target_include_directories(ORNG_CORE PUBLIC ${ORNG_CORE_INCLUDE_DIRS})
target_include_directories(ORNG_RENDERING PUBLIC ${ORNG_CORE_INCLUDE_DIRS})
target_include_directories(ORNG_UTIL PUBLIC ${ORNG_CORE_INCLUDE_DIRS})

target_link_libraries(ORNG_RENDERING PUBLIC
		OpenGL::GL
		glfw
		libglew_shared
		assimp
		${ASSIMP_LIBRARY}
		Tracy::TracyClient
)

target_link_libraries(ORNG_UTIL PUBLIC
	spdlog::spdlog_header_only
)

target_link_libraries(ORNG_CORE PUBLIC
ORNG_RENDERING
ORNG_UTIL
OpenGL::GL
glfw
libglew_shared
assimp
spdlog::spdlog_header_only
yaml-cpp
${YAML_CPP_LIBRARIES}
${ASSIMP_LIBRARY}
${PHYSX_LIBRARIES}
Tracy::TracyClient
)

if (NOT ORNG_DISABLE_FMOD)
	target_link_libraries(ORNG_CORE PUBLIC ${CMAKE_CURRENT_LIST_DIR}/extern/fmod/api/core/lib/x64/fmod$<$<CONFIG:Debug>:L>_vc.lib)
	target_include_directories(ORNG_CORE PUBLIC "${CMAKE_CURRENT_LIST_DIR}/extern/fmod/api/core/inc")
endif()

target_precompile_headers(ORNG_CORE PUBLIC headers/pch/pch.h)
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
CACHE string "Copy these into your .exe directory"
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



target_compile_definitions(ORNG_CORE PUBLIC
ORNG_CORE_MAIN_DIR="${CMAKE_CURRENT_LIST_DIR}"
ORNG_CORE_LIB_DIR="${CMAKE_BINARY_DIR}/ORNG-Core/"
GLM_ENABLE_EXPERIMENTAL
ORNG_CORE_DEBUG_BINARY_DIR="${CMAKE_SOURCE_DIR}/out/build/x64-Debug/"
ORNG_CORE_RELEASE_BINARY_DIR="${CMAKE_SOURCE_DIR}/out/build/x64-Release/"
)

target_compile_definitions(ORNG_RENDERING PUBLIC
		ORNG_CORE_MAIN_DIR="${CMAKE_CURRENT_LIST_DIR}"
		ORNG_CORE_LIB_DIR="${CMAKE_BINARY_DIR}/ORNG-Core/"
		GLM_ENABLE_EXPERIMENTAL
		ORNG_CORE_DEBUG_BINARY_DIR="${CMAKE_SOURCE_DIR}/out/build/x64-Debug/"
		ORNG_CORE_RELEASE_BINARY_DIR="${CMAKE_SOURCE_DIR}/out/build/x64-Release/"
)


if (CMAKE_BUILD_TYPE STREQUAL "Debug")
set_target_properties(${SCRIPT_TARGETS} PROPERTIES 
LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/build/x64-Debug/ORNG-Core"
ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/build/x64-Debug/ORNG-Core"
RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/build/x64-Debug/ORNG-Core"
)
else()
set_target_properties(${SCRIPT_TARGETS} PROPERTIES 
LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/build/x64-Release/ORNG-Core"
ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/build/x64-Release/ORNG-Core"
RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/build/x64-Release/ORNG-Core"
)
endif()


file(COPY ${CMAKE_CURRENT_LIST_DIR}/res/ DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/res/core-res")





