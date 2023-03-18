#pragma once
#include <glew.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include "Camera.h"
#include "InputHandle.h"
#include "TimeStep.h"
#include "Renderer.h"
#include "RendererData.h"


class Application
{
public:
	Application();
	~Application() = default;
	void Init();
	void RenderScene();
	const std::shared_ptr<InputHandle>& GetInputHandle() const { return input_handle; };

private:
	unsigned int m_window_width = RendererData::WINDOW_WIDTH;
	unsigned int m_window_height = RendererData::WINDOW_HEIGHT;

	std::shared_ptr<InputHandle> input_handle = std::make_shared<InputHandle>();
	std::shared_ptr<Camera> p_camera = std::make_shared<Camera>(m_window_width, m_window_height, input_handle);
	Renderer renderer = Renderer(p_camera);

};