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
#include "ShaderHandling.h"
#include "ExtraMath.h"
#include "WorldTransform.h"
#include "Vertex.h"
#include "Texture.h"
#include "MainFramework.h"
#include "KeyboardState.h"
#include "TimeStep.h"
#include "util.h"

MainFramework::MainFramework() {
	GLclampf Red = 0.0f, Green = 0.0f, Blue = 0.0f, Alpha = 0.0f;
	glClearColor(Red, Green, Blue, Alpha);

	WINDOW_WIDTH = 1920;
	WINDOW_HEIGHT = 1080;
	float FOV = 60.0f;
	float zNear = 0.1f;
	float zFar = 100.0f;

	pKeyboardState = new KeyboardState();
	pTimeStep = new TimeStep();
	pCamera = new Camera(WINDOW_WIDTH, WINDOW_HEIGHT, pTimeStep);
	pMesh = new BasicMesh;

	persProjData = { FOV, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, zNear, zFar };

}


bool MainFramework::Init() {

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);

	glEnable(GL_DEPTH_TEST);

	ShaderProgramSource source = ParseShader("res/shaders/Basic.shader");
	unsigned int shader = CreateShader(source.vertexSource, source.fragmentSource);
	GLCall(glUseProgram(shader));

	GLCall(WVPLocation = glGetUniformLocation(shader, "gTransform"));
	ASSERT(WVPLocation != -1);
	GLCall(samplerLocation = glGetUniformLocation(shader, "gSampler"));
	ASSERT(samplerLocation != -1);

	pTexture = new Texture(GL_TEXTURE_2D, "./res/textures/Orange.jpg");

	if (!pTexture->Load()) {
		return false;
	}

	pTexture->Bind(GL_TEXTURE0);
	glUniform1i(samplerLocation, 0);

	if (!pMesh->LoadMesh("./res/meshes/oranges/10195_Orange-L2.obj")) {
		return false;
	}

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
	WorldTransform worldTransform = pMesh->GetWorldTransform();

	glm::fmat4x4 projectionMatrix = ExtraMath::InitPersProjTransform(persProjData);
	glm::fmat4x4 cameraMatrix = pCamera->GetMatrix();

	worldTransform.SetPosition(0.0f, 0.0f, -5.0f);
	worldTransform.SetScale(0.3f, 0.3f, 0.3f);
	glm::fmat4x4 worldMatrix = worldTransform.GetMatrix();

	glm::fmat4x4 WVP = worldMatrix * cameraMatrix * projectionMatrix;
	glUniformMatrix4fv(WVPLocation, 1, GL_TRUE, &WVP[0][0]);

	pMesh->Render();

	glutPostRedisplay();

	glutSwapBuffers();
}

MainFramework::~MainFramework() {
	delete pTimeStep;
	delete pCamera;
	delete pTexture;
	delete pKeyboardState;
}