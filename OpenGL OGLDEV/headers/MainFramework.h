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
#include "Camera.h"
#include "ShaderHandling.h"
#include "ExtraMath.h"
#include "WorldTransform.h"
#include "Texture.h"
#include "KeyboardState.h"
#include "TimeStep.h"

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
	void CallSpecialKeysUp(int key, int x, int y);
	void CallKeysUp(unsigned char key, int x, int y);
	void InitializeGlutCallbacks();
	void PassiveMouseCB(int x, int y);
	int WINDOW_WIDTH;
	int WINDOW_HEIGHT;

private:

	int TransformLocation = -1;
	int SamplerLocation = -1;
	void CreateCubeVAO();
	void CreatePyramindVAO();
	void CompileShaders();
	GLuint cubeVAO = -1;
	GLuint cubeIBO = -1;
	GLuint cubeVBO = -1;

	void AttachShader(const std::string filePath);

	WorldTransform cubeWorldTransform;
	TimeStep* pTimeStep = nullptr;
	Texture* pTexture = nullptr;
	Camera* pCamera = nullptr;
	KeyboardState* pKeyboardState = nullptr;
	PersProjData persProjData;


};