#include "pch/pch.h"
#include "EditorLayer.h"


int main() {
	ORNG::Scene scene;

	ORNG::EditorLayer editor_layer{&scene, "projects/base-project" };
	ORNG::Application application;

	application.layer_stack.PushLayer(&editor_layer);

	ORNG::ApplicationData data{};
	application.Init(data);

	return 0;
}
