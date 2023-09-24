#include "pch/pch.h"
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
#include "assets/AssetManager.h"
#include "core/Input.h"
#include "audio/AudioEngine.h"
#include <glfw/glfw3.h>
#include "util/ExtraUI.h"


namespace ORNG {

	void InitImGui() {
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImGui_ImplGlfw_InitForOpenGL(Window::GetGLFWwindow(), true);
		ImGui_ImplOpenGL3_Init((char*)glGetString(GL_NUM_SHADING_LANGUAGE_VERSIONS));
	}

	void Application::Shutdown() {
		layer_stack.Shutdown();
		Physics::Shutdown();
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

		//IMGUI INIT
		IMGUI_CHECKVERSION();
		InitImGui();
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
			// Update
			glfwPollEvents();
			Events::EventManager::DispatchEvent(update_event);
			Window::Update();

			// Render
			Events::EventManager::DispatchEvent(render_event);
			glfwSwapBuffers(window);

			FrameTiming::UpdateTimeStep();
		}

		// Cleanup
		Shutdown();
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwDestroyWindow(window);
		glfwTerminate();
	}

}