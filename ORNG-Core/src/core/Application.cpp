#include "pch/pch.h"

#include "rendering/Renderer.h"
#include "core/Application.h"
#include "util/Log.h"
#include "core/Window.h"
#include "rendering/SceneRenderer.h"
#include "core/GLStateManager.h"
#include "events/EventManager.h"
#include "core/FrameTiming.h"
#include "physics/Physics.h"
#include "rendering/EnvMapLoader.h"
#include "core/CodedAssets.h"
#include "assets/AssetManager.h"
#include "core/Input.h"
#include "audio/AudioEngine.h"
#include <glfw/glfw3.h>
#include "util/ExtraUI.h"


namespace ORNG {


	void Application::Shutdown() {
		layer_stack.Shutdown();
		Physics::Shutdown();
		AssetManager::OnShutdown();
		Log::Flush();
		//ImGui::DestroyContext();
	}

	void Application::Init() {

		Events::EventManager::Init();
		Log::Init();

		ORNG_CORE_INFO("Initializing core engine");
		Log::Flush();

		Window::Init();
		GLFWwindow* window = Window::GetGLFWwindow();

		GL_StateManager::InitGlew();
		layer_stack.m_imgui_layer.OnInit();
		GL_StateManager::InitGL();
		AudioEngine::Init();
		Input::Init();
		CodedAssets::Init();
		Renderer::Init();
		Physics::Init();
		EnvMapLoader::Init();
		SceneRenderer::Init();
		AssetManager::Init();


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
			Input::OnUpdate();
			glfwPollEvents();
			Events::EventManager::DispatchEvent(update_event);
			Window::Update();
			ExtraUI::OnUpdate();

			// Render
			Events::EventManager::DispatchEvent(render_event);
			glfwSwapBuffers(window);

			FrameTiming::Update();
		}

		// Cleanup
		Shutdown();


		glfwDestroyWindow(window);
		glfwTerminate();
	}

}