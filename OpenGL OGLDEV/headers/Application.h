#pragma once
#include <glew.h>
#include <glfw/glfw3.h>
#include <stdio.h>
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
#include "InputHandle.h"
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
	void Init();
	void RenderScene();
	std::shared_ptr<InputHandle> GetInputHandle() const;


private:

	void ReshapeCB(int w, int h);
	unsigned int m_window_width = RenderData::WINDOW_WIDTH;
	unsigned int m_window_height = RenderData::WINDOW_HEIGHT;
	TimeStep time_step_camera;

	std::shared_ptr<InputHandle> input_handle = std::make_shared<InputHandle>();
	std::shared_ptr<Camera> p_camera = std::make_shared<Camera>(m_window_width, m_window_height, &time_step_camera, input_handle);
	Renderer renderer = Renderer(p_camera);

};