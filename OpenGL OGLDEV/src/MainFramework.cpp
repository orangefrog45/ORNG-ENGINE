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
#include "GLErrorHandling.h"



#define ASSERT(x) if (!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
    x;\
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))

MainFramework::MainFramework() {
    GLclampf Red = 0.0f, Green = 0.0f, Blue = 0.0f, Alpha = 0.0f;
    glClearColor(Red, Green, Blue, Alpha);

	WINDOW_WIDTH = 1920;
	WINDOW_HEIGHT = 1080;
    float FOV = 60.0f;
    float zNear = 0.1f;
    float zFar = 10.0f;

	pKeyboardState = new KeyboardState();
	pTimeStep = new TimeStep();
	pCamera = new Camera(WINDOW_WIDTH, WINDOW_HEIGHT, pTimeStep);

    persProjData = { FOV, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, zNear, zFar };

}

void MainFramework::CreateCubeVAO() {
	GLCall(glGenVertexArrays(1, &cubeVAO));
	GLCall(glBindVertexArray(cubeVAO));

	Vertex Vertices[8];

	glm::fvec2 t00(0.0f, 0.0f);
	glm::fvec2 t01(0.0f, 1.0f);
	glm::fvec2 t10(1.0f, 0.0f);
	glm::fvec2 t11(1.0f, 1.0f);


	Vertices[0] = Vertex(glm::fvec3(0.5f, 0.5f, 0.5f), t00);
	Vertices[1] = Vertex(glm::fvec3(-0.5f, 0.5f, -0.5f), t01);
	Vertices[2] = Vertex(glm::fvec3(-0.5f, 0.5f, 0.5f), t10);
	Vertices[3] = Vertex(glm::fvec3(0.5f, -0.5f, -0.5f), t11);
	Vertices[4] = Vertex(glm::fvec3(-0.5f, -0.5f, -0.5f), t00);
	Vertices[5] = Vertex(glm::fvec3(0.5f, 0.5f, -0.5f), t10);
	Vertices[6] = Vertex(glm::fvec3(0.5f, -0.5f, 0.5f), t01);
	Vertices[7] = Vertex(glm::fvec3(-0.5f, -0.5f, 0.5f), t11);

	//create handle
	GLCall(glGenBuffers(1, &cubeVBO));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, cubeVBO));
	//knows to look in ARRAY_BUFFER for data which is bound
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW));

	//index offset of pos
	GLCall(glEnableVertexAttribArray(0));
	//specify format of "enabled" attribute (pos)
	GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0));

	//tex coords
	GLCall(glEnableVertexAttribArray(1));
	GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float))));

	unsigned int Indices[] = {
	0, 1, 2,
	1, 3, 4,
	5, 6, 3,
	7, 3, 6,
	2, 4, 7,
	0, 7, 6,
	0, 5, 1,
	1, 5, 3,
	5, 0, 6,
	7, 4, 3,
	2, 1, 4,
	0, 2, 7
	};

	GLCall(glGenBuffers(1, &cubeIBO));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeIBO));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW));

	glBindVertexArray(0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


}

bool MainFramework::Init() {
    CreateCubeVAO();

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);
	glBindVertexArray(cubeVAO);

	ShaderProgramSource source = ParseShader("res/shaders/Basic.shader");
	unsigned int shader = CreateShader(source.vertexSource, source.fragmentSource);
	GLCall(glUseProgram(shader));

	GLCall(SamplerLocation = glGetUniformLocation(shader, "gSampler"));
	ASSERT(SamplerLocation != -1);
	GLCall(TransformLocation = glGetUniformLocation(shader, "gTransform"));
	ASSERT(TransformLocation != -1);

	pTexture = new Texture(GL_TEXTURE_2D, "./res/textures/Orange.jpg");

	if (!pTexture->Load()) {
		return false;
	}

	pTexture->Bind(GL_TEXTURE0);
	glUniform1i(SamplerLocation, 0);

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
	GLCall(glClear(GL_COLOR_BUFFER_BIT));

	pTimeStep->timeInterval = glutGet(GLUT_ELAPSED_TIME) - pTimeStep->lastTime;
	pTimeStep->lastTime = glutGet(GLUT_ELAPSED_TIME);

	cubeWorldTransform.SetPosition(0.0f, 0.0f, -5.0f);
	pCamera->HandleInput(pKeyboardState);

	glm::fmat4x4 projectionMatrix = ExtraMath::InitPersProjTransform(persProjData);
	glm::fmat4x4 worldMatrix = cubeWorldTransform.GetMatrix();
	glm::fmat4x4 cameraMatrix = pCamera->GetMatrix();

	glm::fmat4x4 WVP = worldMatrix * cameraMatrix * projectionMatrix;

	GLCall(glUniformMatrix4fv(TransformLocation, 1, GL_TRUE, &WVP[0][0]));

	//for querying current vao
	GLint currentVAO;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);

	GLCall(glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0));

	glutPostRedisplay();

	glutSwapBuffers();
}
MainFramework::~MainFramework() {
	delete pTimeStep;
	delete pCamera;
	delete pTexture;
	delete pKeyboardState;
}