#pragma once
#include "layers/EditorLayer.h"


class Application
{
public:
	Application();
	~Application() = default;
	void Init();
	void RenderScene();

private:
	EditorLayer editor_layer = EditorLayer();
};