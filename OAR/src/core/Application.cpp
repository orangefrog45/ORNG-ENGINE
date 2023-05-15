#include "pch/pch.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <glfw/glfw3.h>
#include "rendering/Renderer.h"
#include "core/Application.h"
#include "core/Input.h"
#include "util/Log.h"
#include "core/Window.h"

namespace ORNG {

	Application::Application() {
	}


	void Application::Init() {
		//GLFW INIT

		if (!glfwInit())
			exit(1);

		Window::Init();
		GLFWwindow* window = Window::GetGLFWwindow();

		glfwMakeContextCurrent(window);
		glfwSwapInterval(0);
		glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE); // keys "stick" until they've been polled


		//GLEW INIT
		GLint GlewInitResult = glewInit();

		if (GLEW_OK != GlewInitResult)
		{
			OAR_CORE_CRITICAL("GLEW INIT FAILED");
			printf("ERROR: %s", glewGetErrorString(GlewInitResult));
			exit(EXIT_FAILURE);
		}

		//IMGUI INIT
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init((char*)glGetString(GL_NUM_SHADING_LANGUAGE_VERSIONS));

		//GL CONFIG
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
		glCullFace(GL_BACK);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glShadeModel(GL_SMOOTH);

		Input::Init(window);

		glDebugMessageCallback(Log::GLLogMessage, nullptr);
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

		Renderer::Init();
		editor_layer.Init();

		/* Loop until the user closes the window */

		while (!glfwWindowShouldClose(window))
		{

			Input::UpdateTimeStep();
			/* Render here */
			editor_layer.ShowDisplayWindow();
			editor_layer.ShowUIWindow();

			glfwSwapBuffers(window);
			glfwPollEvents();
			editor_layer.Update();

		}

		// Cleanup
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void Application::RenderScene() {

	}

}