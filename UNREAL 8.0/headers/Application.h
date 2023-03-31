#pragma once
#include "Camera.h"
#include "InputHandle.h"
#include "Renderer.h"
#include "RendererResources.h"


class Application
{
public:
	Application();
	~Application() = default;
	void Init();
	void RenderScene();

private:
	InputHandle input_handle;
	Renderer renderer;
};