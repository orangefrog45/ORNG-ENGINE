#include "../includes/ScriptAPI.h"
#include <GLFW/glfw3.h>
#include <jolt/Jolt.h>
#include <jolt/Core/Factory.h>

#include "core/FrameTiming.h"
#include "events/EventManager.h"
#include "core/Window.h"
#include "core/GLStateManager.h"
#include "rendering/Renderer.h"
#include "assets/AssetManager.h"
#include "imgui/imgui.h"


extern "C" {
	// Connect main application's singletons with dll
	__declspec(dllexport) void SetSingletonPtrs(void* p_window, void* p_frametiming, void* p_event_manager,
		void* p_gl_manager, void* p_asset_manager, void* p_renderer, void* p_logger, void* p_ringbuffer_sink, void* p_phys_factory) {
		ORNG::Log::InitFrom(*static_cast<std::shared_ptr<spdlog::logger>*>(p_logger), *static_cast<std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt>*>(p_ringbuffer_sink));
		ORNG::Window::InitInstance(static_cast<ORNG::Window*>(p_window));
		ORNG::FrameTiming::Init(static_cast<ORNG::FrameTiming*>(p_frametiming));
		ORNG::Events::EventManager::SetInstance(static_cast<ORNG::Events::EventManager*>(p_event_manager));
		ORNG::GL_StateManager::Init(static_cast<ORNG::GL_StateManager*>(p_gl_manager));
		ORNG::AssetManager::Init(static_cast<ORNG::AssetManager*>(p_asset_manager));
		ORNG::Renderer::Init(static_cast<ORNG::Renderer*>(p_renderer));
		JPH::Factory::sInstance = static_cast<JPH::Factory*>(p_phys_factory);


		// Also initialize glfw for window stuff
		glfwInit();
	}

	// Connect main application's ImGui context with dll
	__declspec(dllexport) void SetImGuiContext(void* p_instance, void* mem_alloc_func, void* free_func) {
		ImGui::SetCurrentContext(static_cast<ImGuiContext*>(p_instance));
		ImGui::SetAllocatorFunctions(static_cast<ImGuiMemAllocFunc>(mem_alloc_func), static_cast<ImGuiMemFreeFunc>(free_func));
	}

	__declspec(dllexport) void Unload() {
		glfwMakeContextCurrent(nullptr);
		glfwTerminate();
	}
}
