#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <glew.h>
#include <glfw/glfw3.h>
#include <stdio.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_major_storage.hpp>
#include <sstream>
#include <fstream>
#include <iostream>
#include <stb/stb_image.h>
#include <random>
#include "Camera.h"
#include "ExtraMath.h"
#include "WorldTransform.h"
#include "Texture.h"
#include "Application.h"
#include "TimeStep.h"
#include "util/util.h"
#include "Renderer.h"
#include "Skybox.h"



Application::Application() {
}


void Application::Init() {
	//GLFW INIT
	GLFWwindow* window;

	if (!glfwInit())
		exit(1);

	window = glfwCreateWindow(m_window_width, m_window_height, "UNREAL 8.0", NULL, NULL);

	if (!window)
	{
		glfwTerminate();
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE); // keys "stick" until they've been polled

	//GLEW INIT
	GLint GlewInitResult = glewInit();

	if (GLEW_OK != GlewInitResult)
	{
		printf("ERROR: %s", glewGetErrorString(GlewInitResult));
		exit(EXIT_FAILURE);
	}

	//IMGUI INIT
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	io.FontDefault = io.Fonts->AddFontFromFileTTF("./res/fonts/PlatNomor-WyVnn.ttf", 18.0f);

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init((char*)glGetString(GL_NUM_SHADING_LANGUAGE_VERSIONS));

	//GL CONFIG
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(3.0);

	//RENDERER INIT
	renderer.Init();


	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		/* Render here */
		RenderScene();

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

		input_handle->HandleInput(window, m_window_width, m_window_height); // handle mouse-locking, caching key states
		p_camera->HandleInput();

		{ // debug spawn entities
			static int instances = 0;

			if (input_handle->g_pressed) {
				auto entity = renderer.scene.CreateMeshEntity("./res/meshes/cube/cube.obj");
				glm::fvec3 place_position = p_camera->GetPos() + (glm::fvec3(p_camera->GetTarget().x * 15.0f, p_camera->GetTarget().y * 15.0f, p_camera->GetTarget().z * 15.0f));
				instances++;
				entity->SetPosition(place_position.x, place_position.y, place_position.z);

				PrintUtils::PrintDebug("Instances: " + std::to_string(instances));
			}
		}

	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::RenderScene() {

	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	renderer.RenderScene();

}
