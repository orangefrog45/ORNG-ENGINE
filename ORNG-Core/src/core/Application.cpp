#include "pch/pch.h"

#include "rendering/Renderer.h"
#include "core/Application.h"

#include <imgui_internal.h>

#include "util/Log.h"
#include "core/Window.h"
#include "rendering/SceneRenderer.h"
#include "core/GLStateManager.h"
#include "events/EventManager.h"
#include "core/FrameTiming.h"
#include "assets/AssetManager.h"
#include "audio/AudioEngine.h"
#include <glfw/glfw3.h>
#include "util/ExtraUI.h"

using namespace ORNG;

void Application::Shutdown() {
	layer_stack.Shutdown();

	Renderer::Shutdown();
	Window::Shutdown();

	ImGui::Shutdown();

	if (!(m_settings.disabled_modules & ApplicationModulesFlags::ASSET_MANAGER))
		AssetManager::Shutdown();

	glfwTerminate();
	GL_StateManager::Shutdown();
}

void Application::Init(const ApplicationData& data) {
	m_settings = data;

	if (!glfwInit()) {
		ORNG_CORE_CRITICAL("GLFW Failed to initialize");
		exit(1);
	}

	Log::Init();

	Events::EventManager::Init();
	FrameTiming::Init();
	Window::InitInstance();
	Window::Init(data.initial_window_dimensions, data.window_name,  data.initial_window_display_monitor_idx,
		data.window_iconified, data.window_decorated, data.start_maximized);
	GLFWwindow* window = Window::GetGLFWwindow();
	GL_StateManager::Init();
	GL_StateManager::InitGlew();
	layer_stack.m_imgui_layer.OnInit();
	GL_StateManager::InitGL();

	if (!(data.disabled_modules & ApplicationModulesFlags::AUDIO))
		AudioEngine::Init();

	Renderer::Init();

	if (!(data.disabled_modules & ApplicationModulesFlags::ASSET_MANAGER))
		AssetManager::Init();

#ifdef ORNG_ENABLE_TRACY_PROFILE
	TracyGpuContext(Window::GetGLFWwindow());
#endif

	ORNG_CORE_INFO("Core engine initialized, intializing layers");
	layer_stack.Init();
	ORNG_CORE_INFO("Layers initialized, beginning main loop");

	bool running = true;

	Events::EventListener<Events::EngineCoreEvent> termination_listener{};
	termination_listener.OnEvent = [&](const Events::EngineCoreEvent& _event) {
		if (_event.event_type == Events::EngineCoreEvent::REQUEST_TERMINATE) running = false;
	};
	Events::EventManager::RegisterListener(termination_listener);

	// Engine loop
	while (running && !glfwWindowShouldClose(window)) {
		glfwPollEvents();
		Events::EventManager::DispatchEvent(Events::EngineCoreEvent{.event_type = Events::EngineCoreEvent::FRAME_START});
		Events::EventManager::DispatchEvent(Events::EngineCoreEvent{.event_type = Events::EngineCoreEvent::BEFORE_UPDATE});
		Events::EventManager::DispatchEvent(Events::EngineCoreEvent{.event_type = Events::EngineCoreEvent::UPDATE});
		Events::EventManager::DispatchEvent(Events::EngineCoreEvent{.event_type = Events::EngineCoreEvent::POST_UPDATE});
		Events::EventManager::DispatchEvent(Events::EngineCoreEvent{.event_type = Events::EngineCoreEvent::BEFORE_RENDER});
		Events::EventManager::DispatchEvent(Events::EngineCoreEvent{.event_type = Events::EngineCoreEvent::RENDER});
		Events::EventManager::DispatchEvent(Events::EngineCoreEvent{.event_type = Events::EngineCoreEvent::POST_RENDER});
		Events::EventManager::DispatchEvent(Events::EngineCoreEvent{.event_type = Events::EngineCoreEvent::FRAME_END});

		glfwSwapBuffers(window);

#ifdef ORNG_ENABLE_TRACY_PROFILE
		TracyGpuCollect;
#endif
		FrameTiming::Update();
		Window::Update(); // Window must update last for input state to be valid
	}

	Events::EventManager::DeregisterListener(termination_listener.GetRegisterID());

	// Cleanup
	Shutdown();	
}
