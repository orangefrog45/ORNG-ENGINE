#include "pch/pch.h"
#include "EditorLayer.h"


void main() {
	ORNG::EditorLayer editor_layer;
	ORNG::Application application;
	application.layer_stack.PushLayer(static_cast<ORNG::Layer*>(&editor_layer));
	application.Init();
}