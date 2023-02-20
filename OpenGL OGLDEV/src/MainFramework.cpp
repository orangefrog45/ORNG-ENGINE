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

	camera = Camera(WINDOW_WIDTH, WINDOW_HEIGHT, &timeStep);
	currentFrames = 0;
	lastFrames = 0;

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);

	glEnable(GL_DEPTH_TEST);

	meshLibrary.Init();
	skybox.Init();


	/*meshArray[basicProgramIndex].push_back(new BasicMesh);
	if (!meshArray[basicProgramIndex][0]->LoadMesh("./res/meshes/robot/robot.obj")) {
		return false;
	}*/


	meshLibrary.lightingShaderMeshes.push_back(BasicMesh(50000));
	if (!(meshLibrary.lightingShaderMeshes[0].LoadMesh("./res/meshes/Rock1/rock2.obj"))) {
		return false;
	}


	auto& transforms = meshLibrary.lightingShaderMeshes[0].GetWorldTransforms();
	float x = 0.0f;
	float y = 0.0f;
	float z = -10.0f;
	float angle = 0.0f;

	for (unsigned int i = 0; i < transforms.size(); i++) {
		angle += 10.0f;
		x += 4 * cosf(ExtraMath::ToRadians(angle));
		y += 4 * sinf(ExtraMath::ToRadians(angle));
		if (angle == 360) {
			angle = 0.0f;
			x = 0.0f;
			y = 0.0f;
			z -= 2.0f;
		}
		/*if (y == 32.0f) {
			y = 0.0f;
			z += 4.0f;
		}
		if (z == 32.0f) {
			z == 0.0f;
		}*/
		transforms[i].SetPosition(x, y, z);
		transforms[i].SetScale(0.4f, 0.4f, 0.4f);
	}
	/*std::uniform_int_distribution<> distr(-10.0f, 10.0f);
	glm::fvec3 pos = transforms[i].GetPosition();
	offset += -20.0f;
	transforms[i].SetPosition(pos.x + offset, pos.y + offset, -50.0f + offset);*/

	/*WorldTransform& worldTransform = meshArray[basicProgramIndex][0]->GetWorldTransform();
	worldTransform.SetPosition(0.0f, 0.0f, -5.0f);
	worldTransform.SetScale(0.01f, 0.01f, 0.01f);*/

	WorldTransform& worldTransform2 = meshLibrary.lightingShaderMeshes[0].GetWorldTransforms()[0];
	worldTransform2.SetPosition(10.0f, 10.0f, -10.0f);
	worldTransform2.SetScale(0.1f, 0.1f, 0.1f);


	return true;

}

void MainFramework::CallKeysUp(unsigned char key, int x, int y) {
	keyboardState.KeysUp(key, x, y);
}

void MainFramework::CallSpecialKeysUp(int key, int x, int y) {
	keyboardState.SpecialKeysUp(key, x, y);
}

void MainFramework::CallKeyboardCB(unsigned char key, int x, int y) {
	keyboardState.KeyboardCB(key, x, y);
}

void MainFramework::CallSpecialKeyboardCB(int key, int x, int y) {
	keyboardState.SpecialKeyboardCB(key, x, y);
}

void MainFramework::PassiveMouseCB(int x, int y)
{
	camera.OnMouse(glm::vec2(x, y));
}


void MainFramework::ReshapeCB(int w, int h) {
	WINDOW_WIDTH = w;
	WINDOW_HEIGHT = h;
}

void MainFramework::MonitorFrames() {
	currentFrames++;

	if (glutGet(GLUT_ELAPSED_TIME) - timeStepFrames.lastTime > 1000) {
		timeStepFrames.lastTime = glutGet(GLUT_ELAPSED_TIME);
		FPS = currentFrames - lastFrames;
		lastFrames = currentFrames;
		printf("FPS: %s\n", std::to_string(FPS).c_str());
	}

}

void MainFramework::RenderSceneCB() {

	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	MonitorFrames();
	timeStep.timeInterval = glutGet(GLUT_ELAPSED_TIME) - timeStep.lastTime;
	timeStep.lastTime = glutGet(GLUT_ELAPSED_TIME);

	camera.HandleInput(&keyboardState);

	glm::fmat4x4 projectionMatrix = ExtraMath::InitPersProjTransform(persProjData);
	glm::fmat4 cameraTransMatrix = ExtraMath::GetCameraTransMatrix(camera.GetPos());
	//skybox.Draw(cameraTransMatrix * camera.GetMatrix() * projectionMatrix);

	auto& transforms = meshLibrary.lightingShaderMeshes[0].GetWorldTransforms();
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float angle = 0.0f;
	static double offset = 0.0f;
	float rowOffset = 0.0f;
	static float coefficient = 1.0f;

	for (unsigned int i = 0; i < transforms.size(); i++) {
		angle += 10.0f;
		x += 4 * cosf(ExtraMath::ToRadians(angle + rowOffset)) * coefficient;
		y += 4 * sinf(ExtraMath::ToRadians(angle + rowOffset)) *- coefficient;
		if (angle == 180.0f) {
			coefficient *= -1.0;
		}
		if (angle >= 360) {
			coefficient *= 1.0f;
			angle = 0.0f;
			z -= 4.0f;
			x = 0.0f;
			y = 0.0f;
			rowOffset += 9.0f;
		}
		offset += 0.00005f;
		transforms[i].SetPosition((cosf(ExtraMath::ToRadians(offset)) * x) - (sinf(ExtraMath::ToRadians(offset)) * y), (sinf(ExtraMath::ToRadians(offset)) * x) + (cosf(ExtraMath::ToRadians(offset)) * y), z);
		transforms[i].SetScale(0.4f, 0.4f, 0.4f);

	}

	WorldData data = WorldData(camera.GetMatrix(), cameraTransMatrix, projectionMatrix);

	meshLibrary.RenderAllMeshes(data);

	glutPostRedisplay();

	glutSwapBuffers();
}

MainFramework::~MainFramework() {
}