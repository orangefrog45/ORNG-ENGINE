#pragma once
#include "InputHandle.h"
#include "Renderer.h"
#include "EditorLayer.h"


class Application
{
public:
	Application();
	~Application() = default;
	void Init();
	void RenderScene();

private:
	InputHandle input_handle;
	Renderer m_renderer;
	EditorLayer editor_layer = EditorLayer(&m_renderer);
};