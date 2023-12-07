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
		//ImGui::DestroyContext();
	}

	void Application::Init() {

		Events::EventManager::Init();
		Log::Init();
		Window::Init();
		GLFWwindow* window = Window::GetGLFWwindow();


		//GLEW INIT
		GLint GlewInitResult = glewInit();

		if (GLEW_OK != GlewInitResult)
		{
			ORNG_CORE_CRITICAL("GLEW INIT FAILED");
			printf("ERROR: %s", glewGetErrorString(GlewInitResult));
			exit(EXIT_FAILURE);
		}

		// Have to do up here for now to prevent a crash
		layer_stack.m_imgui_layer.OnInit();
		AudioEngine::Init();
		Input::Init();
		GL_StateManager::InitGL();
		CodedAssets::Init();
		Renderer::Init();
		Physics::Init();
		EnvMapLoader::Init();
		SceneRenderer::Init();
		AssetManager::Init();
		layer_stack.Init();


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

			FrameTiming::UpdateTimeStep();
		}

		// Cleanup
		Shutdown();


		glfwDestroyWindow(window);
		glfwTerminate();
	}

}