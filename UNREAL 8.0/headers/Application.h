#pragma once
#include "Camera.h"
#include "InputHandle.h"
#include "Renderer.h"
#include "RendererData.h"


class Application
{
public:
	Application();
	~Application() = default;
	void Init();
	void RenderScene();

private:
	unsigned int m_window_width = RendererData::WINDOW_WIDTH;
	unsigned int m_window_height = RendererData::WINDOW_HEIGHT;

	InputHandle input_handle;
	std::shared_ptr<Camera> p_camera = std::make_shared<Camera>(m_window_width, m_window_height);
	Renderer renderer = Renderer(p_camera);

};