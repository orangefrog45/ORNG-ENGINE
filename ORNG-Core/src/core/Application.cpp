#include "pch/pch.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
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
#include "core/AssetManager.h"
#include "core/Input.h"
#include <glfw/glfw3.h>


namespace ORNG {

	Application::Application() {
	}


	void Application::Init() {
		Log::Init();
		//GLFW INIT

		if (!glfwInit())
			exit(1);

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

		//IMGUI INIT
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init((char*)glGetString(GL_NUM_SHADING_LANGUAGE_VERSIONS));

		Events::EventManager::Init();
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
			// Update
			glfwPollEvents();
			Events::EventManager::ProcessQueuedEvents();
			Events::EventManager::DispatchEvent(update_event);
			Window::Update();

			// Render
			Events::EventManager::DispatchEvent(render_event);
			glfwSwapBuffers(window);

			FrameTiming::UpdateTimeStep();
		}

		// Cleanup
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwDestroyWindow(window);
		glfwTerminate();
	}

}