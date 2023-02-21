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
#include "shaders/LightingShader.h"
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
	void PassiveMouseCB(int x, int y);
	KeyboardState& GetKeyboard();
	unsigned int GetWindowWidth() const { return m_window_width; };
	unsigned int GetWindowHeight() const { return m_window_height; };


private:

	void MonitorFrames();
	unsigned int m_current_frames;
	unsigned int m_last_frames;
	unsigned int m_fps;
	TimeStep time_step_camera;
	TimeStep time_step_frames;
	Camera camera;
	Skybox skybox;
	KeyboardState keyboardState;
	MeshLibrary meshLibrary;
	PersProjData persProjData;
	unsigned int m_window_width = 1920;
	unsigned int m_window_height = 1080;

};