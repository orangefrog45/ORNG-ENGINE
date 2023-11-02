#include "../../ORNG-Editor/headers/EditorLayer.h"
#include "../headers/GameLayer.h"

void main() {
	ORNG::Application application;
	ORNG::EditorLayer editor;
	ORNG::GameLayer game;
	application.layer_stack.PushLayer(static_cast<ORNG::Layer*>(&editor));
	application.layer_stack.PushLayer(static_cast<ORNG::Layer*>(&game));
	application.Init();
}