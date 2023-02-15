#pragma once
#include <stdio.h>
#include <glew.h>
#include <freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <sstream>
#include <fstream>
#include <iostream>
#include <stb/stb_image.h>
#include <vector>
#include "Camera.h"
#include "ExtraMath.h"
#include "WorldTransform.h"
#include "Texture.h"
#include "BasicMesh.h"
#include "KeyboardState.h"
#include "ShaderLibrary.h"
#include "TimeStep.h"
#include "Skybox.h"
#include "MeshLibrary.h"


class MainFramework
{
public:
	MainFramework();
	~MainFramework();

	bool Init();

	void RenderSceneCB();
	void ReshapeCB(int w, int h);
	void CallKeyboardCB(unsigned char key, int mouse_x, int mouse_y);
	void CallSpecialKeyboardCB(int key, int mouse_x, int mouse_y);
	void MonitorFrames();
	void CallSpecialKeysUp(int key, int x, int y);
	void CallKeysUp(unsigned char key, int x, int y);
	void PassiveMouseCB(int x, int y);
	int WINDOW_WIDTH;
	int WINDOW_HEIGHT;

private:

	unsigned int currentFrames;
	unsigned int lastFrames;
	unsigned int FPS;
	TimeStep timeStep;
	TimeStep timeStepFrames;
	Camera camera;
	Skybox skybox;
	KeyboardState keyboardState;
	MeshLibrary meshLibrary;
	PersProjData persProjData;

};