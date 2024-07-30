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
#include "rendering/EnvMapLoader.h"
#include "assets/AssetManager.h"
#include "core/Input.h"
#include "audio/AudioEngine.h"
#include <glfw/glfw3.h>
#include "util/ExtraUI.h"


namespace ORNG {

	void Application::Shutdown() {
		ImGui::Shutdown();
		layer_stack.Shutdown();

		if (!(m_settings.disabled_modules & ApplicationModulesFlags::PHYSICS))
			Physics::Shutdown();

		if (!(m_settings.disabled_modules & ApplicationModulesFlags::ASSET_MANAGER))
			AssetManager::OnShutdown();

		Log::Flush();
	}

	void Application::Init(const ApplicationData& data) {
		m_settings = data;

		if (!glfwInit()) {
			ORNG_CORE_CRITICAL("GLFW Failed to initialize");
			exit(1);
		}

		Events::EventManager::Init();
		Log::Init();

		ORNG_CORE_INFO("Initializing core engine");
		Log::Flush();
		Window::Init(data.initial_window_dimensions, data.window_name, data.initial_window_display_monitor_idx, data.window_iconified, data.window_decorated);
		GLFWwindow* window = Window::GetGLFWwindow();

		GL_StateManager::InitGlew();
		layer_stack.m_imgui_layer.OnInit();
		GL_StateManager::InitGL();

		if (!(data.disabled_modules & ApplicationModulesFlags::AUDIO))
			AudioEngine::Init();

		if (!(data.disabled_modules & ApplicationModulesFlags::INPUT))
			Input::Init();

#ifdef ORNG_RUNTIME
		Renderer::GetShaderLibrary().LoadShaderPackage(data.shader_package_file);
#endif
		Renderer::Init();

		if (!(data.disabled_modules & ApplicationModulesFlags::PHYSICS))
			Physics::Init();

		if (!(data.disabled_modules & ApplicationModulesFlags::SCENE_RENDERER))
			SceneRenderer::Init();

		if (!(data.disabled_modules & ApplicationModulesFlags::ASSET_MANAGER))
			AssetManager::Init();

#ifdef ORNG_ENABLE_TRACY_PROFILE
		TracyGpuContext(Window::GetGLFWwindow());
#endif

		ORNG_CORE_INFO("Core engine initialized, intializing layers");
		layer_stack.Init();
		ORNG_CORE_INFO("Layers initialized, beginning main loop");

		// Engine loop
		Events::EngineCoreEvent render_event;
		render_event.event_type = Events::Event::ENGINE_RENDER;

		Events::EngineCoreEvent update_event;
		update_event.event_type = Events::Event::ENGINE_UPDATE;

		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			Input::OnUpdate();
			Events::EventManager::DispatchEvent(update_event);
			ExtraUI::OnUpdate();

			// Render
			Events::EventManager::DispatchEvent(render_event);
			glfwSwapBuffers(window);

#ifdef ORNG_ENABLE_TRACY_PROFILE
			TracyGpuCollect;
#endif

			FrameTiming::Update();
		}

		// Cleanup
		Shutdown();
		glfwDestroyWindow(window);
		glfwTerminate();
	}

}