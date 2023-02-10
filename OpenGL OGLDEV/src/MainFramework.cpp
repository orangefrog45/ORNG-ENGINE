#include <stdio.h>
#include <glew.h>
#include <freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
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
#include "ShaderLibrary.h"

MainFramework::MainFramework() {
	GLclampf Red = 0.0f, Green = 0.0f, Blue = 0.0f, Alpha = 0.0f;
	glClearColor(Red, Green, Blue, Alpha);

	WINDOW_WIDTH = 1920;
	WINDOW_HEIGHT = 1080;
	float FOV = 60.0f;
	float zNear = 0.1f;
	float zFar = 100.0f;


	persProjData = { FOV, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, zNear, zFar };

}


bool MainFramework::Init() {

	pKeyboardState = new KeyboardState();
	pTimeStep = new TimeStep();
	pCamera = new Camera(WINDOW_WIDTH, WINDOW_HEIGHT, pTimeStep);
	pShaderLibrary = new ShaderLibrary();

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);

	glEnable(GL_DEPTH_TEST);

	pShaderLibrary->Init();
	pShaderLibrary->ActivateProgram(pShaderLibrary->programIDs[basicProgramIndex]);

	glUniform1i(samplerLocation, 0);

	meshArray[basicProgramIndex].push_back(new BasicMesh);
	if (!meshArray[basicProgramIndex][0]->LoadMesh("./res/meshes/oranges/10195_Orange-L2.obj")) {
		return false;
	}

	meshArray[testProgramIndex].push_back(new BasicMesh);
	if (!meshArray[testProgramIndex][0]->LoadMesh("./res/meshes/oranges/10195_Orange-L2.obj")) {
		return false;
	}

	WorldTransform& worldTransform = meshArray[basicProgramIndex][0]->GetWorldTransform();
	worldTransform.SetPosition(0.0f, 0.0f, -5.0f);
	worldTransform.SetScale(0.3f, 0.3f, 0.3f);

	WorldTransform& worldTransform2 = meshArray[testProgramIndex][0]->GetWorldTransform();
	worldTransform2.SetPosition(17.0f, 0.0f, -5.0f);
	worldTransform2.SetScale(0.3f, 0.3f, 0.3f);

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


void MainFramework::RenderSceneCB() {

	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	pTimeStep->timeInterval = glutGet(GLUT_ELAPSED_TIME) - pTimeStep->lastTime;
	pTimeStep->lastTime = glutGet(GLUT_ELAPSED_TIME);

	pCamera->HandleInput(pKeyboardState);

	glm::fmat4x4 projectionMatrix = ExtraMath::InitPersProjTransform(persProjData);

	for (unsigned int i = 0; i < meshArray.size(); i++) {
		//iterate over arrays of meshes with same program, i == shader position in programIDs
		pShaderLibrary->ActivateProgram(pShaderLibrary->programIDs[i]);

		GLCall(WVPLocation = glGetUniformLocation(pShaderLibrary->programIDs[i], "gTransform"));
		ASSERT(WVPLocation != -1);
		GLCall(samplerLocation = glGetUniformLocation(pShaderLibrary->programIDs[i], "gSampler"));
		ASSERT(samplerLocation != -1);

		glm::fmat4x4 cameraMatrix = pCamera->GetMatrix();

		for (unsigned int y = 0; y < meshArray[i].size(); y++) {

			WorldTransform worldTransform = meshArray[i][y]->GetWorldTransform();

			glm::fmat4x4 WVP = worldTransform.GetMatrix() * cameraMatrix * projectionMatrix;
			glUniformMatrix4fv(WVPLocation, 1, GL_TRUE, &WVP[0][0]);

			meshArray[i][y]->Render();
		}
	}

	glutPostRedisplay();

	glutSwapBuffers();
}

MainFramework::~MainFramework() {
	delete pTimeStep;
	delete pCamera;
	delete pKeyboardState;
	delete pShaderLibrary;

	for (unsigned int i = 0; i < meshArray.size(); i++) {
		for (unsigned int y = 0; y < meshArray[i].size(); y++) {
			delete meshArray[i][y];
		}
	}
}