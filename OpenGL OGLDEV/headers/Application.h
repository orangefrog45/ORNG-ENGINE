#pragma once
#include <stdio.h>
#include <glew.h>
#include <freeglut.h>
#include <glm/glm.hpp>
#include <fstream>
#include <memory>
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
#include "Renderer.h"


class Application
{
public:
	Application();
	~Application();
	bool Init();
	std::shared_ptr<KeyboardState> GetKeyboard();
	void PassiveMouseCB(int x, int y);
	void RenderSceneCB();


private:

	void MonitorFrames();
	void InitGlutCallbacks();
	void ReshapeCB(int w, int h);
	unsigned int m_current_frames;
	unsigned int m_last_frames;
	unsigned int m_window_width = RenderData::WINDOW_WIDTH;
	unsigned int m_window_height = RenderData::WINDOW_HEIGHT;
	TimeStep time_step_camera;
	TimeStep time_step_frames;

	std::shared_ptr<KeyboardState> keyboard_state = std::make_shared<KeyboardState>();
	std::shared_ptr<Camera> p_camera = std::make_shared<Camera>(m_window_width, m_window_height, &time_step_camera, keyboard_state);
	Renderer renderer = Renderer(p_camera);

};