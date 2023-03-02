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
#include "KeyboardState.h"
#include "TimeStep.h"
#include "util/util.h"
#include "Renderer.h"
#include "Skybox.h"
#include <imgui/imgui.h>



Application::Application() {
}


void Application::Init() {
	m_current_frames = 0;
	m_last_frames = 0;
	GLFWwindow* window;

	if (!glfwInit())
		exit(1);


	window = glfwCreateWindow(m_window_width, m_window_height, "UNREAL 8.0", NULL, NULL);

	if (!window)
	{
		glfwTerminate();
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	GLint GlewInitResult = glewInit();
	if (GLEW_OK != GlewInitResult)
	{
		printf("ERROR: %s", glewGetErrorString(GlewInitResult));
		exit(EXIT_FAILURE);
	}

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(3.0);
	renderer.Init();

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT);
		RenderScene();

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glfwTerminate();
}

void Application::PassiveMouseCB(int x, int y)
{
	p_camera->OnMouse(glm::vec2(x, y));
}

std::shared_ptr<KeyboardState> Application::GetKeyboard() {
	return keyboard_state;
}


void Application::ReshapeCB(int w, int h) {
	m_window_width = w;
	m_window_height = h;
}

void Application::MonitorFrames() {
	m_current_frames++;

	if (glfwGetTime() - time_step_frames.lastTime > 1000) {
		time_step_frames.lastTime = glfwGetTime();
		PrintUtils::PrintDebug("FPS: " + std::to_string(m_current_frames - m_last_frames));
		m_last_frames = m_current_frames;
	}

}

void Application::RenderScene() {

	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	MonitorFrames();
	time_step_camera.timeInterval = glfwGetTime() - time_step_camera.lastTime;
	time_step_camera.lastTime = glfwGetTime();

	p_camera->HandleInput();
	static int instances = 0;
	if (keyboard_state->g_pressed) {
		auto entity = renderer.scene.CreateMeshEntity("./res/meshes/cube/cube.obj");
		glm::fvec3 place_position = p_camera->GetPos() + (glm::fvec3(-p_camera->GetTarget().x * 15.0f, -p_camera->GetTarget().y * 15.0f, -p_camera->GetTarget().z * 15.0f));
		instances++;
		entity->SetPosition(place_position.x, place_position.y, place_position.z);

		PrintUtils::PrintDebug("Instances: " + std::to_string(instances));
	}

	renderer.RenderScene();

}

Application::~Application() {
}