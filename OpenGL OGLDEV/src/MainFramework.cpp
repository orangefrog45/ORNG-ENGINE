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
#include "MainFramework.h"
#include "KeyboardState.h"
#include "TimeStep.h"
#include "util.h"
#include "MeshLibrary.h"
#include "Skybox.h"

MainFramework::MainFramework() {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	constexpr float FOV = 60.0f;
	constexpr float zNear = 0.01f;
	constexpr float zFar = 1000.0f;

	persProjData = { FOV, (float)m_window_width, (float)m_window_height, zNear, zFar };

}


bool MainFramework::Init() {
	camera = Camera(m_window_width, m_window_height, &time_step_camera);
	m_current_frames = 0;
	m_last_frames = 0;

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	meshLibrary.Init();
	skybox.Init();

	meshLibrary.lightingShaderMeshes.emplace_back(BasicMesh(50000));
	if (!(meshLibrary.lightingShaderMeshes[0].LoadMesh("./res/meshes/Rock1/rock2.obj"))) {
		return false;
	}


	return true;
}

void MainFramework::PassiveMouseCB(int x, int y)
{
	camera.OnMouse(glm::vec2(x, y));
}

KeyboardState& MainFramework::GetKeyboard() {
	return keyboardState;
}


void MainFramework::ReshapeCB(int w, int h) {
	m_window_width = w;
	m_window_height = h;
}

void MainFramework::MonitorFrames() {
	m_current_frames++;

	if (glutGet(GLUT_ELAPSED_TIME) - time_step_frames.lastTime > 1000) {
		time_step_frames.lastTime = glutGet(GLUT_ELAPSED_TIME);
		m_fps = m_current_frames - m_last_frames;
		m_last_frames = m_current_frames;
		printf("FPS: %s\n", std::to_string(m_fps).c_str());
	}

}

void MainFramework::RenderSceneCB() {

	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	MonitorFrames();
	time_step_camera.timeInterval = glutGet(GLUT_ELAPSED_TIME) - time_step_camera.lastTime;
	time_step_camera.lastTime = glutGet(GLUT_ELAPSED_TIME);

	camera.HandleInput(keyboardState);

	meshLibrary.AnimateGeometry();

	glm::fmat4x4 projectionMatrix = ExtraMath::InitPersProjTransform(persProjData);
	glm::fmat4 cameraTransMatrix = ExtraMath::GetCameraTransMatrix(camera.GetPos());
	//skybox.Draw(cameraTransMatrix * camera.GetMatrix() * projectionMatrix);
	ViewData data = ViewData(camera.GetMatrix(), cameraTransMatrix, projectionMatrix, camera.GetPos());

	meshLibrary.RenderAllMeshes(data);

	glutPostRedisplay();

	glutSwapBuffers();
}

MainFramework::~MainFramework() {
}