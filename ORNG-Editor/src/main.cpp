#include "pch/pch.h"
#include "EditorLayer.h"

struct InitializationLayer : public ORNG::Layer {
	void OnInit() override {
		scene = std::make_unique<ORNG::Scene>();
	}

	void OnRender() override {};
	void OnImGuiRender() override {};
	void Update() override {};
	void OnShutdown() override {};

	std::unique_ptr<ORNG::Scene> scene = nullptr;

};

void main() {
	InitializationLayer il{};
	ORNG::EditorLayer editor_layer{&il.scene, "projects/base-project" };
	ORNG::Application application;

	application.layer_stack.PushLayer(&il);
	application.layer_stack.PushLayer(static_cast<ORNG::Layer*>(&editor_layer));

	ORNG::ApplicationData data{};
	application.Init(data);
}