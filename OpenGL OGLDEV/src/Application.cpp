#include <stdio.h>
#include <glew.h>
#include <freeglut.h>
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

Application::Application() {
}


bool Application::Init() {
	m_current_frames = 0;
	m_last_frames = 0;

	renderer.Init();


	return true;
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

	if (glutGet(GLUT_ELAPSED_TIME) - time_step_frames.lastTime > 1000) {
		time_step_frames.lastTime = glutGet(GLUT_ELAPSED_TIME);
		PrintUtils::PrintDebug("FPS: " + std::to_string(m_current_frames - m_last_frames));
		m_last_frames = m_current_frames;
	}

}

void Application::RenderSceneCB() {

	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	MonitorFrames();
	time_step_camera.timeInterval = glutGet(GLUT_ELAPSED_TIME) - time_step_camera.lastTime;
	time_step_camera.lastTime = glutGet(GLUT_ELAPSED_TIME);

	p_camera->HandleInput();
	static int instances = 0;
	if (keyboard_state->g_pressed) {
		auto entity = renderer.scene.CreateMeshEntity("./res/meshes/oranges/orange.obj");
		glm::fvec3 place_position = p_camera->GetPos() + (glm::fvec3(-p_camera->GetTarget().x * 15.0f, -p_camera->GetTarget().y * 15.0f, -p_camera->GetTarget().z * 15.0f));
		instances++;
		entity->SetPosition(place_position.x, place_position.y, place_position.z);

		PrintUtils::PrintDebug("Instances: " + std::to_string(instances));
	}

	//renderer.AnimateGeometry();

	renderer.RenderScene();

	glutPostRedisplay();

	glutSwapBuffers();
}

Application::~Application() {
}