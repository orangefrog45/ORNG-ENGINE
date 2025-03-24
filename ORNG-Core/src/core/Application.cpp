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
#include "physics/Physics.h"
#include "assets/AssetManager.h"
#include "core/Input.h"
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

	if (!(m_settings.disabled_modules & ApplicationModulesFlags::PHYSICS))
		Physics::Shutdown();

	glfwDestroyWindow(Window::GetGLFWwindow());
	glfwTerminate();

	//GL_StateManager::Shutdown();
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

	if (!(data.disabled_modules & ApplicationModulesFlags::PHYSICS))
		Physics::Init();

	if (!(data.disabled_modules & ApplicationModulesFlags::ASSET_MANAGER))
		AssetManager::Init();

#ifdef ORNG_ENABLE_TRACY_PROFILE
	TracyGpuContext(Window::GetGLFWwindow());
#endif

	ORNG_CORE_INFO("Core engine initialized, intializing layers");
	layer_stack.Init();
	ORNG_CORE_INFO("Layers initialized, beginning main loop");

	Events::EngineCoreEvent render_event;
	render_event.event_type = Events::EngineCoreEvent::ENGINE_RENDER;

	Events::EngineCoreEvent update_event;
	update_event.event_type = Events::EngineCoreEvent::ENGINE_UPDATE;

	// Engine loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		Events::EventManager::DispatchEvent(update_event);
		ExtraUI::OnUpdate(); // TODO: get this out of here
			
		Events::EventManager::DispatchEvent(render_event);

		glfwSwapBuffers(window);

#ifdef ORNG_ENABLE_TRACY_PROFILE
		TracyGpuCollect;
#endif
		FrameTiming::Update();
		Window::Update(); // Window must update last for input state to be valid
	}

	// Cleanup
	Shutdown();	
}
