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
	GLclampf Red = 0.0f, Green = 0.0f, Blue = 0.0f, Alpha = 0.0f;
	glClearColor(Red, Green, Blue, Alpha);

	WINDOW_WIDTH = 1920;
	WINDOW_HEIGHT = 1080;
	float FOV = 60.0f;
	float zNear = 0.01f;
	float zFar = 1000.0f;


	persProjData = { FOV, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, zNear, zFar };

}


bool MainFramework::Init() {

	pKeyboardState = new KeyboardState();
	pTimeStep = new TimeStep();
	pTimeStepFrames = new TimeStep();
	pCamera = new Camera(WINDOW_WIDTH, WINDOW_HEIGHT, pTimeStep);
	pSkybox = new Skybox();
	currentFrames = 0;
	lastFrames = 0;

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);

	glEnable(GL_DEPTH_TEST);

	meshLibrary.Init();
	//pSkybox->Init();

	/*meshArray[basicProgramIndex].push_back(new BasicMesh);
	if (!meshArray[basicProgramIndex][0]->LoadMesh("./res/meshes/robot/robot.obj")) {
		return false;
	}*/
	meshLibrary.basicShaderMeshes.push_back(BasicMesh());
	if (!(meshLibrary.basicShaderMeshes[0].LoadMesh("./res/meshes/oranges/10195_Orange-L2.obj"))) {
		return false;
	}

	/*WorldTransform& worldTransform = meshArray[basicProgramIndex][0]->GetWorldTransform();
	worldTransform.SetPosition(0.0f, 0.0f, -5.0f);
	worldTransform.SetScale(0.01f, 0.01f, 0.01f);*/

	WorldTransform& worldTransform2 = meshLibrary.basicShaderMeshes[0].GetWorldTransform();
	worldTransform2.SetPosition(10.0f, 10.0f, -10.0f);
	worldTransform2.SetScale(0.1f, 0.1f, 0.1f);


	return true;

}

void MainFramework::CallKeysUp(unsigned char key, int x, int y) {
	pKeyboardState->KeysUp(key, x, y);
}

void MainFramework::CallSpecialKeysUp(int key, int x, int y) {
	pKeyboardState->SpecialKeysUp(key, x, y);
}

void MainFramework::CallKeyboardCB(unsigned char key, int x, int y) {
	pKeyboardState->KeyboardCB(key, x, y);
}

void MainFramework::CallSpecialKeyboardCB(int key, int x, int y) {
	pKeyboardState->SpecialKeyboardCB(key, x, y);
}

void MainFramework::PassiveMouseCB(int x, int y)
{
	pCamera->OnMouse(glm::vec2(x, y));
}


void MainFramework::ReshapeCB(int w, int h) {
	WINDOW_WIDTH = w;
	WINDOW_HEIGHT = h;
}

void MainFramework::MonitorFrames() {
	currentFrames++;

	if (glutGet(GLUT_ELAPSED_TIME) - pTimeStepFrames->lastTime > 1000) {
		pTimeStepFrames->lastTime = glutGet(GLUT_ELAPSED_TIME);
		FPS = currentFrames - lastFrames;
		lastFrames = currentFrames;
		printf("FPS: %s\n", std::to_string(FPS).c_str());
	}

}

void MainFramework::RenderSceneCB() {

	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	MonitorFrames();
	pTimeStep->timeInterval = glutGet(GLUT_ELAPSED_TIME) - pTimeStep->lastTime;
	pTimeStep->lastTime = glutGet(GLUT_ELAPSED_TIME);

	pCamera->HandleInput(pKeyboardState);

	glm::fmat4x4 projectionMatrix = ExtraMath::InitPersProjTransform(persProjData);
	glm::fmat4 cameraTransMatrix = ExtraMath::GetCameraTransMatrix(pCamera->GetPos());
	pSkybox->Draw();

	meshLibrary.RenderBasicShaderMeshes(WorldData(pCamera->GetMatrix(), cameraTransMatrix, projectionMatrix));


	glutPostRedisplay();

	glutSwapBuffers();
}

MainFramework::~MainFramework() {
	delete pTimeStep;
	delete pTimeStepFrames;
	delete pCamera;
	delete pKeyboardState;
	delete pSkybox;
}